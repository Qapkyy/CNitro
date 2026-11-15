obj-m += src/acer-wmi-battery.o
obj-m += src/acer-wmi-fan.o
obj-m += src/acer-wmi-profile.o

ccflags-y += -I$(src)/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
