#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/dmi.h>
#include <linux/fixp-arith.h>
#include <linux/backlight.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/platform_profile.h>
#include <linux/acpi.h>
#include <linux/i8042.h>
#include <linux/rfkill.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <acpi/video.h>
#include <linux/hwmon.h>
#include <linux/units.h>
#include <linux/unaligned.h>
#include <linux/bitfield.h>
#include <linux/bitmap.h>

enum acer_wmi_event_ids {
	WMID_HOTKEY_EVENT = 0x1;
	WMID_BACKLIGHT_EVENT = 0x4;
	WMID_ACCEL_OR_KBD_DOCK_EVENT = 0x5;
	WMID_GAMING_TURBO_KEY_EVENT = 0x7;
	WMID_AC_EVENT = 0x8;
};

enum acer_wmi_nitro_sys_info_command {

};

enum acer_wmi_nitro_sensor_id {

};

enum acer_wmi_nitro_gaming_fan_id {

};

enum acer_wmi_nitro_gaming_fan_mode {

};

enum acer_wmi_nitro_oc {

};

enum acer_wmi_nitro_gaming_misc_setting {

};

static const struct key_entry acer_wmi_keymap[] __initconst = {

};

static struct input_dev *acer_wmi_input_dev;
static struct input_dev *acer_wmi_accel_dev;

struct event_return_value {} __packed;

struct func_input_params {} __packed;

struct wmid3_gds_set_input_param {} __packed;

struct wmid3_gds_get_input_param {} __packed;

struct wmid3_gds_return_value {} __packed;

struct hotkey_function_type_aa {} __packed;

enum interface_flags {

};

static int max_brightness = 0xF;
static int mailled = -1;
static int brightness = -1;
static int threeg = -1;
static int force_series;
static int force_caps = -1;
static bool ec_raw_mode;
static bool has_type_aa;
static u16 commun_func_bitmap;
static u8 commun_fn_key_number;
static bool cycle_gaming_thermal_profile = true;
static bool nitro;
static u64 supported_sensors;

module_param(mailled, int, 0444);
module_param(brightness, int, 0444);
module_param(threeg, int, 0444);
module_param(force_series, int, 0444);
module_param(force_caps, int, 0444);
module_param(ec_raw_mode, bool, 0444);
module_param(cycle_gaming_thermal_profile, bool, 0644);
module_param(nitro, bool, 0444);
MODULE_PARM_DESC(mailled, "Set initial state of Mail LED");
MODULE_PARM_DESC(brightness, "Set initial LCD backlight brightness");
MODULE_PARM_DESC(threeg, "Set initial state of 3G hardware");
MODULE_PARM_DESC(force_series, "Force a different laptop series");
MODULE_PARM_DESC(force_caps, "Force the capability bitmask to this value");
MODULE_PARM_DESC(ec_raw_mode, "Enable EC raw mode");
MODULE_PARM_DESC(cycle_gaming_thermal_profile, "Set thermal mode key in cycle mode. Disabling it sets the mode key in turbo toggle mode");
MODULE_PARM_DESC(nitro, "Enable features for predator laptops that use predator sense v4");

struct acer_data {

};

struct acer_debug {

};

static struct rfkill *wireless_rfkill;
static struct rfkill *bluetooth_rfkill;
static struct rfkill *threeg_rfkill;
static bool rfkill_inited;

struct wmi_interface {

};

static struct wmi_interface *interface;

struct quirk_entry {

};

static struct quirk_entry *quirk;

static void __init set_quirks(void)
{

}

static int __init dmi_matched(const struct dmi_system_id *dmi)
{

}

static int __init set_force_caps(const struct dmi_system_id *dmi)
{

}

static struct quirk_entry quirk_unknown = {
};

/* Sesuai kan dengan model laptop */













static const struct dmi_system_id amw0_whitelist[] __initconst = {
	{
		.ident = "Acer",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Acer"),
		},
	},
	{
		.ident = "Gateway",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Gateway"),
		},
	},
	{
		.ident = "Packard Bell",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Packard Bell"),
		},
	},
	{}
};

static const struct dmi_system_id acer_quirks[] __initconst = {

};

static const struct dmi_system_id non_acer_quirks[] __initconst = {

};

static struct device *platform_profile_device;
static bool platform_profile_support;

static int last_non_turbo_profile = INT_MIN;

enum acer_nitro_thermal_profile {
	
};

static void __init find_quirks(void)
{

}

static bool has_cap(u32 cap)
{

}

struct wmab_args {

};

struct wmab_ret {

};

static acpi_status wmab_execute(struct wmab_args *regbuf,
struct acpi_buffer *result)
{

}

static acpi_status AMW0_get_u32(u32 *value, u32 cap)
{

}

static acpi_status AMW0_set_u32(u32 value, u32 cap)
{

}

static acpi_status __init AMW0_find_mailled(void)
{

}

static const struct acpi_device_id norfkill_ids[] __initconst = {

};

static int __init AMW0_set_cap_acpi_check_device(void)
{

}

static acpi_status __init AMW0_set_capabilities(void)
{

}

static struct wmi_interface AMW0_interface = {

};

static struct wmi_interface AMW0_V2_interface = {

};

static acpi_statuc WMI_execute_u32(u32 method_id, u32 in, u32 *out)
{

}

static acpi_status WMID_get_u32(u32 *value, u32 cap)
{

}

static acpi_status WMID_set_u32(u32 value, u32 cap)
{

}

static acpi_status wmid3_get_device_status(u32 *value, u16 device)
{

}

static acpi_status wmid_v2_get_u32(u32 *value, u32 cap)
{

}

static acpi_status wmid3_set_device_status(u32 value, u16 device)
{

}

static acpi_status wmid_v2_set_u32(u32 value, u32 cap)
{

}

static void __init type_aa_dmi_decode(const struct dmi_header *header, void *d)
{

}

static acpi_status __init WMID_set_capabilities(void)
{

}

static struct wmi_interface wmid_interface = {

};

static struct wmi_interface wmid_v2_interface = {

};

static acpi_status WMI_gaming_execute_u64(u32 method_id, u64 in, u64 *out)
{

}

static int WMI_gaming_execute_u32_u64(u32 method_id, u32 in, u64 *out)
{

}

static acpi_status WMID_gaming_set_u64(u64 value, u32 cap)
{

}

static acpi_status WMID_gaming_get_u64(u64 *value, u32 cap)
{

}

static int WMID_gaming_get_sys_info(u32 command, u64 *out)
{

}

static int WMID_gaming_set_fan_behavior(u16 fan_bitmap, enum acer_wmi_gaming_fan_mode mode)
{

}

static int WMID_gaming_get_fan_behavior(u16 fan_bitmap, enum acer_wmi_gaming_fan_mode *mode)
{

}

static void WMID_gaming_set_fan_mode(enum acer_wmi_gaming_fan_mode mode)
{

}

static int WMID_gaming_set_gaming_fan_speed(u8 fan, u8 speed)
{

}

static int WMID_gaming_get_gaming_fan_speed(u8 fan, u8 *speed)
{

}

static int WMID_gaming_set_misc_setting(enum acer_wmi_gaming_misc_setting setting, u8 value)
{

}

static int WMID_gaming_get_misc_setting(enum acer_wmi_gaming_misc_setting setting, u8 *value)
{

}

static acpi_status get_u32(u32 *value, u32 cap)
{

}

static acpi_status set_u32(u32 value, u32 cap)
{

}

static void __init acer_commandline_init(void)
{

}

static void mail_led_set(struct led_classdev *led_cdev, enum led_brightness value)
{

}

static struct led_classdev mail_led = {

};

static int acer_led_init(struct device *dev)
{

}

static void acer_led_exit(void)
{

}

static struct backlight_device *acer_backlight_device;

static int read_brightness(struct backlight_devie *bd)
{

}

static int update_bl_status(struct backlight_device *bd)
{

}

static const struct backlight_ops acer_bl_ops = {

};

static int acer_backlight_init(struct device *dev)
{

}

static void acer_backlight_exit(void)
{

}

static acpi_handle gsensor_handle;

static int acer_gsensor_init(void)
{

}

static int acer_gsensor_open(struct input_dev *input)
{

}

static int acer_gsensor_event(void)
{

}

/* Predator series turbo button */

static int acer_toggle_turbon(void)
{

}

static int acer_nitro_platform_profile_get(struct device *dev,
					   enum platform_profile_option *profile)
{

}

static int acer_nitro_platform_profile_set(struct device *dev,
					   enum platform_profile_option profile)
{

}

static int acer_nitro_platform_profile_probe(void *drvdata, unsigned long *choices)
{

}

static const struct platform_profile_ops acer_nitro_platform_profile_ops = {

};

static int acer_platform_profile_setup(struct platform_device *device)
{

}

static int acer_thermal_profile_change(void)
{

}

static int acer_kbd_dock_state_to_sw_tablet_mode(u8 kbd_docs_state)
{

}

static void acer_kbd_dock_get_initial_state(void)
{

}

static void acer_kbd_dock_event(const struct event_return_value *event)
{

}

static void acer_rfkill_update(struct work_struct *ignored);
static DECLARE_DELAYED_WORK(acer_rfkill_work, acer_rfkill_update);
static void acer_rfkill_update(struct work_struct *ignored)
{

}

static int acer_rfkill_set(void *data, bool blocked)
{

}

static const struct rfkill_ops acer_rfkill_ops = {

};

static struct rfkill *acer_rfkill_register(struct device *dev,
					   enum rfkill_type type,
					   char *name, u32 cap)
{

}

static int acer_rfkill_init(struct device *dev)
{

}

static void acer_rfkill_exit(void)
{

}

static void acer_wmi_notify(union acpi_object *obj, void *context)
{

}

static acpi_status __init wmid3_set_function_mode(struct func_input_params *params,
						  struct func_return_value *return_value)
{

}

static int __init acer_wmi_enable_ec_raw(void)
{

}


static int __init acer_wmi_enable_lm(void)
{

}

static int __init acer_wmi_enable_rf_button(void)
{

}

static int __init acer_wmi_accel_setup(void)
{

}

static int __init acer_wmi_input_setup(void)
{

}

static void acer_wmi_input_destroy(void)
{

}

/* debugfs functions */
static u32 get_wmid_devices(void)
{

}

static int acer_wmi_hwmon_init(void);

static int acer_platform_probe(struct platform_device *device)
{

}

static void acer_platform_remove(struct platfrom_device *device)
{

}

#ifdef CONFIG_PM_SLEEP
static int acer_suspend(struct device *dev)
{

}

static int acer_resume(struct device *dev)
{

}

#else
#define acer_suspend NULL
#define acer_resume NULL
#endif

static SIMPLE_DEV_PM_OPS(acer_pm, acer_suspend, acer_resume);

static void acer_platform_shutdown(struct platform_device *device)
{

}

static struct platform_driver acer_platform_driver = {

};

static struct platform_device *acer_platform_device;

static void remove_debugfs(void)
{

}

static void __init create_debugfs(void)
{

}

static const enum acer_wmi_nitro_sensor_id acer_wmi_temp_channel_to_sensor_id[] = {

};

static const enum acer_wmi_nitro_sensor_id acer_wmi_fan_channel_to_sensor_id[] = {

};

static const enum acer_wmi_gaming_fan_id acer_wmi_fan_channel_to_fan_id[] = {

};

static const u16 acer_wmi_fan_channel_to_fan_bitmap[] = {

};

static umode_t acer_wmi_hwmon_is_visible(const void *data,
					 enum hwmon_sensor_types type, u32 attr,
					 int channel)
{

}

static int acer_wmi_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			       u32 attr, int channel, long *val)
{

}

static int acer_wmi_hwmon_write(struct device *dev, enum hwmon_sensor_types type,
				u32 attr, int channel, long val)
{

}

static const struct hwmon_channel_info *const acer_wmi_hwmon_info[] = {

};

static const struct hwmon_ops acer_wmi_hwmon_ops = {

};

static const struct hwmon_chip_info acer_wmi_hwmon_chip_info = {

};

static int acer_wmi_hwmon_init(void)
{

}

static int __init acer_wmi_init(void)
{

}

static void __exit acer_wmi_exit(void)
{

}

module_init(acer_wmi_init);
module_exit(acer_wmi_exit);

MODULE_AUTHOR("Qapky");
MODULE_DESCRIPTION("Acer Nitro V16 Family WMI Extras Driver");
MODULE_LICENSE("GPL");
