#
# Makefile for the sensors drivers.
#

# Each configuration option enables a list of files.

ccflags-$(CONFIG_SENSORS_FPRINT_SECURE) := -DENABLE_SENSORS_FPRINT_SECURE
ccflags-y += -Wformat

obj-$(CONFIG_SENSORS_FINGERPRINT)	+= fingerprint_sysfs.o
obj-$(CONFIG_SENSORS_QBT2000)		+= qbt2000_common.o qbt2000_platform.o qbt2000_spidev.o
obj-$(CONFIG_SENSORS_GW9558X)		+= gw9558x_common.o gw9558x_platform.o gw9558x_spidev.o
