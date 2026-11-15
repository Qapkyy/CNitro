// SPDX-License-Identifier: GPL-2.0
/*
 * acer-wmi-fan.c: Acer Nitro Fan Control
 *
 * This is a driver for the WMI battery health control interface found
 * on some Acer laptops.
 * 
 * Based on acer_acpi: WMID 7A4DDFE7-5B5D-40B4-8595-4408E0CC7F56
 *		       ACPI Method is \_SB.PC00.WMID.WMBH
 *
 * Copyright (C) Mulky Fauzan <qapkyy3@gmail.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/acpi.h>
#include <linux/build_bug.h>
#include <linux/dmi.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/byteorder.h>

MODULE_AUTHOR("Mulky Fauzan <qapkyy3@gmail.com>");
MODULE_DESCRIPTION("Acer fan control WMI driver");
MODULE_LICENSE("GPL");

#define METHOD_FAN_BEHAVIOR     14
#define METHOD_FAN_SPEED        16
#define METHOD_SYS_INFO         5

#define SYSINFO_CPU_FAN_RPM     0x0201
#define SYSINFO_GPU_FAN_RPM     0x0601
#define SYSINFO_STATUS_MASK     0xffULL         
#define SYSINFO_RPM_SHIFT       8
#define SYSINFO_RPM_MASK        0x1fffULL       

#define FAN_IDX_CPU             1
#define FAN_IDX_GPU             4

#define SEL_CPU                 0x1
#define SEL_GPU                 0x8

#define MODE_AUTO               1
#define MODE_MAX                2
#define MODE_CUSTOM             3

#define CPU_MODE_SHIFT          16
#define GPU_MODE_SHIFT          22

#define PCT_MAX                 100
#define PCT_SAFE_MIN            20

#define SPEED_CMD(percent, fan_idx) (((u64)(percent) << 8) | (fan_idx))
#define BEHAVIOR_CMD(sel, cpu_mode, gpu_mode) \
	((u64)sel | ((u64)cpu_mode << CPU_MODE_SHIFT) | \
	 ((u64)gpu_mode << GPU_MODE_SHIFT))

static bool force;
static bool unsafe_low_duty;

module_param(force, bool, 0444);
module_param(unsafe_low_duty, bool, 0600);
MODULE_PARM_DESC(force, "Skip the DMI check (dangerous on unknown hardware)");
MODULE_PARM_DESC(unsafe_low_duty,
		 "Allow fixed speeds of 1-19% (risk of overheating). Must come before fan_speed on the insmod command line");

static DEFINE_MUTEX(fan_lock);
static acpi_handle wmbh_handle;
static bool hw_ready, touched, have_pending;       
static int pending_cpu, pending_gpu;
static int cur_cpu, cur_gpu; 

static u64 result_to_u64(const union acpi_object *obj)
{
        __le64 tmp = 0;
        size_t n;

        if (!obj)
                return 0;

        if (obj->type == ACPI_TYPE_INTEGER)
                return obj->integer.value;

        if (obj->type == ACPI_TYPE_BUFFER) {
                n = min_t(size_t, obj->buffer.length, sizeof(tmp));
                memcpy(&tmp, obj->buffer.pointer, n);
                return le64_to_cpu(tmp);
        }

        return 0;
}

/*
 * One WMBH call. fan_lock must be held. If out is non-NULL the raw
 * firmware return value is stored there; otherwise a non-zero return
 * value is only logged.
 */
static int wmbh_call(u32 method_id, u64 value, u64 *out)
{
	union acpi_object args[3];
	acpi_status status;
	u64 ret;
	
	struct acpi_object_list input = {
		.count = ARRAY_SIZE(args),
		.pointer = args,
	};

	struct acpi_buffer output = {
		.length = ACPI_ALLOCATE_BUFFER,
		.pointer = NULL,
	};

	__le64 data = cpu_to_le64(value);
	lockdep_assert_held(&fan_lock);
	memset(args, 0, sizeof(args));

	args[0].type = ACPI_TYPE_INTEGER;       
        args[0].integer.value = 0;

        args[1].type = ACPI_TYPE_INTEGER;       
        args[1].integer.value = method_id;

        args[2].type = ACPI_TYPE_BUFFER;        
        args[2].buffer.length = sizeof(data);
        args[2].buffer.pointer = (u8 *)&data;

	status = acpi_evaluate_object(wmbh_handle, NULL, &input, &output);
        if (ACPI_FAILURE(status)) {
		/* WMBH */
                pr_err("method=%u, value=0x%llx failed: %s\n", 
		       method_id, (unsigned long long) value, acpi_format_exception(status));
                return -EIO;
        }

        ret = result_to_u64(output.pointer);
        kfree(output.pointer);

        pr_debug("WMBH(method=%u, value=0x%llx) -> 0x%llx\n",
                 method_id, (unsigned long long)value,
                 (unsigned long long)ret);

        if (out) {
                *out = ret;
        } else if (ret) {
                /* Meaning of the return value is unknown; log for diagnosis. */
                pr_info("firmware returned 0x%llx for method %u (informational)\n", (unsigned long long)ret, method_id);
        }

        return 0;
}

static void restore_auto_locked(void)
{
        if (wmbh_call(METHOD_FAN_BEHAVIOR,
                      BEHAVIOR_CMD(SEL_CPU | SEL_GPU, MODE_AUTO, MODE_AUTO), NULL))
                pr_warn("failed to restore automatic fan mode\n");
        else
                pr_info("fans returned to automatic mode\n");
}

struct fan_step {
	u32 method;
	u64 value;
};

static int build_steps(int cpu, int gpu, struct fan_step steps[3])
{
        int n = 0;

        if (!cpu && !gpu) {
                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_CPU | SEL_GPU, MODE_AUTO, MODE_AUTO) 
		};
        } else if (cpu == PCT_MAX && gpu == PCT_MAX) {
                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_CPU | SEL_GPU, MODE_MAX, MODE_MAX) 
		};
        } else if (!cpu) {
                /* CPU automatic, GPU fixed */
                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_CPU, MODE_AUTO, 0) 
		};
                
		steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_GPU, 0, MODE_CUSTOM) 
		};
                
		steps[n++] = (struct fan_step){ 
			METHOD_FAN_SPEED,
                        SPEED_CMD(gpu, FAN_IDX_GPU) 
		};
        } else if (!gpu) {
                /* GPU automatic, CPU fixed */
                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_GPU, 0, MODE_AUTO) 
		};

                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_CPU, MODE_CUSTOM, 0) 
		};

                steps[n++] = (struct fan_step){ 
			METHOD_FAN_SPEED,
                        SPEED_CMD(cpu, FAN_IDX_CPU) 
		};
        } else {
                /* Both fixed */
                steps[n++] = (struct fan_step){ 
			METHOD_FAN_BEHAVIOR,
                        BEHAVIOR_CMD(SEL_CPU | SEL_GPU, MODE_CUSTOM, MODE_CUSTOM) 
		};

                steps[n++] = (struct fan_step){ 
			METHOD_FAN_SPEED,
                        SPEED_CMD(cpu, FAN_IDX_CPU) 
		};

                steps[n++] = (struct fan_step){ 
			METHOD_FAN_SPEED,
                        SPEED_CMD(gpu, FAN_IDX_GPU) 
		};
        }

        return n;
}

/*
 * Apply a validated request. fan_lock must be held and hw_ready set.
 * If a step fails midway, fall back to automatic mode so a fan is never
 * left in an undefined custom state.
 */
static int apply_locked(int cpu, int gpu)
{
        struct fan_step steps[3];
        int i, n, ret = 0;

        lockdep_assert_held(&fan_lock);

        n = build_steps(cpu, gpu, steps);
        touched = true;

        for (i = 0; i < n; i++) {
                ret = wmbh_call(steps[i].method, steps[i].value, NULL);
                if (ret)
                        break;
        }

        if (ret) {
                pr_err("step %d/%d failed, restoring automatic fan mode\n",
                       i + 1, n);
                restore_auto_locked();
                cur_cpu = 0;
                cur_gpu = 0;
                return ret;
        }

        cur_cpu = cpu;
        cur_gpu = gpu;
        pr_info("fan speed set: CPU=%d GPU=%d\n", cpu, gpu);
        return 0;
}

static const struct dmi_system_id acer_fan_dmi_table[] __initconst = {
	{
		.ident = "Acer Nitro ANV16-71",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
			DMI_MATCH(DMI_PRODUCT_NAME, "Nitro ANV16-71"),
		},
	}, 
	{},
};


static int probe_locked(void)
{
        acpi_status status;

        lockdep_assert_held(&fan_lock);

        if (hw_ready)
                return 0;

        if (!force && !dmi_check_system(acer_fan_dmi_table)) {
                pr_err("unsupported machine (expected Acer Nitro ANV16-71); use force=1 to override\n");
                return -ENODEV;
        }

        status = acpi_get_handle(NULL, "\\_SB.PC00.WMID.WMBH", &wmbh_handle);
        if (ACPI_FAILURE(status)) {
                pr_err("cannot find ACPI method %s: %s\n",
                       "\\_SB.PC00.WMID.WMBH", acpi_format_exception(status));
                wmbh_handle = NULL;
                return -ENODEV;
        }

        hw_ready = true;
        return 0;
}

static int validate_pct(int v, const char *name)
{
        if (v < 0 || v > PCT_MAX) {
                pr_warn("%s speed %d out of range (0-%d)\n",
                        name, v, PCT_MAX);
                return -EINVAL;
        }

        if (v > 0 && v < PCT_SAFE_MIN && !unsafe_low_duty) {
                pr_warn("%s speed %d%% below safe minimum %d%%; set unsafe_low_duty=1 to allow\n",
                        name, v, PCT_SAFE_MIN);
                return -EINVAL;
        }

        return 0;
}

static int parse_speeds(const char *val, int *cpu, int *gpu)
{
        char buf[16];
        char *p, *tok;
        int ret;

        if (!val || strscpy(buf, val, sizeof(buf)) < 0)
                return -EINVAL;

        p = strim(buf);
        tok = strsep(&p, ",");
        if (!tok || !p)
                return -EINVAL;

        ret = kstrtoint(strim(tok), 10, cpu);
        if (ret)
                return ret;

        ret = kstrtoint(strim(p), 10, gpu);
        if (ret)
                return ret;

        ret = validate_pct(*cpu, "CPU");
        if (ret)
                return ret;

        return validate_pct(*gpu, "GPU");
}

static int fan_speed_set(const char *val, const struct kernel_param *kp)
{
        int cpu, gpu, ret;

        ret = parse_speeds(val, &cpu, &gpu);
        if (ret)
                return ret;

        mutex_lock(&fan_lock);
        if (!hw_ready) {
                pending_cpu = cpu;
                pending_gpu = gpu;
                have_pending = true;
                ret = 0;
        } else {
                ret = apply_locked(cpu, gpu);
        }
        mutex_unlock(&fan_lock);

        return ret;
}

static int fan_speed_get(char *buffer, const struct kernel_param *kp)
{
        int cpu, gpu;

        mutex_lock(&fan_lock);
        cpu = cur_cpu;
        gpu = cur_gpu;
        mutex_unlock(&fan_lock);

        return scnprintf(buffer, PAGE_SIZE, "%d,%d\n", cpu, gpu);
}

static int read_rpm_locked(u32 command, u32 *rpm)
{
        u64 raw = 0;
        int ret;

        lockdep_assert_held(&fan_lock);

        ret = wmbh_call(METHOD_SYS_INFO, command, &raw);
        if (ret)
                return ret;

        if (raw & SYSINFO_STATUS_MASK) {
                pr_warn("sys info query 0x%04x failed, raw=0x%llx\n",
                        command, (unsigned long long)raw);
                return -EIO;
        }

        *rpm = (raw >> SYSINFO_RPM_SHIFT) & SYSINFO_RPM_MASK;
        return 0;
}

static int fan_rpm_get(char *buffer, const struct kernel_param *kp)
{
        u32 cpu_rpm = 0, gpu_rpm = 0;
        int ret;

        mutex_lock(&fan_lock);
        if (!hw_ready) {
                ret = -ENODEV;
        } else {
                ret = read_rpm_locked(SYSINFO_CPU_FAN_RPM, &cpu_rpm);
                if (!ret)
                        ret = read_rpm_locked(SYSINFO_GPU_FAN_RPM, &gpu_rpm);
        }
        mutex_unlock(&fan_lock);

        if (ret)
                return ret;

        return scnprintf(buffer, PAGE_SIZE, "%u,%u\n", cpu_rpm, gpu_rpm);
}

static int fan_rpm_set(const char *val, const struct kernel_param *kp)
{
        return -EPERM;  /* read-only */
}

static const struct kernel_param_ops fan_rpm_ops = {
        .set = fan_rpm_set,
        .get = fan_rpm_get,
};

static const struct kernel_param_ops fan_speed_ops = {
        .set = fan_speed_set,
        .get = fan_speed_get,
};

module_param_cb(fan_speed, &fan_speed_ops, NULL, 0600);
MODULE_PARM_DESC(fan_speed,
                 "\"cpu,gpu\" fan speed in percent: 0 = auto, 20-99 = fixed, 100 = max");

module_param_cb(fan_rpm, &fan_rpm_ops, NULL, 0400);
MODULE_PARM_DESC(fan_rpm, "Read-only: current \"cpu,gpu\" fan RPM");

static int __init acer_fan_init(void)
{
        int ret;

        mutex_lock(&fan_lock);
        ret = probe_locked();
        if (!ret && have_pending)
                ret = apply_locked(pending_cpu, pending_gpu);
        mutex_unlock(&fan_lock);

        if (ret)
                return ret;

        return 0;
}

static void __exit acer_fan_exit(void)
{
        mutex_lock(&fan_lock);
        if (hw_ready && touched)
                restore_auto_locked();
        hw_ready = false;
        mutex_unlock(&fan_lock);

        pr_info("Unload Module.\n");
}

module_init(acer_fan_init);
module_exit(acer_fan_exit);

MODULE_AUTHOR("Mulky Fauzan <qapkyy3@gmail.com>");
MODULE_DESCRIPTION("Acer fan control WMI driver");
MODULE_LICENSE("GPL");
