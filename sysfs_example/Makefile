# We will use this same Makefile for two purposes:
# - Start the build process locally by typing make in the directory that this file
#   is sitting in.
# - Pass the build instructions to the kernel of what files we want built.
#
# Since we will be passing this directory to the kernel as a subdirectory we want
# processed during the build process, the kernel will be looking for a 'Makefile'
# (exact same filename) to know what files need to be built, we might as well use
# the same Makefile.
# Given the fact that then the kernel build system is calling this file, we know
# that the macro KERNELRELEASE will be defined as part of its build system, we use
# that knoledge to split up what parts of this file should be a normal stand-alone
# Makefile (any format we want), and what parts need to be in the format the kernel
# expects them.

ifneq ($(KERNELRELEASE),)

# This is the version that the kernel build scripts will see.
obj-m	:= sysfs_example.o
module-objs := sysfs_example.o


else
# This is the version that will be seen when you type make in your local directory.

ifeq ("$(CROSS_COMPILE)","")
	CC_CHECK:=echo !!ERROR!! CROSS_COMPILE not set
endif

# Kernel Source Directory
KDIR		:= $(OUTDIR)/linux-3.14

# Cross Compiler Prefix and ARCH
#CROSS_COMPILE	:= arm-linux-gnueabihf-
#ARCH		:= arm

# Location of Driver Source
DRVDIR		:= $(shell pwd)

MAKE := make

export ARCH CROSS_COMPILE


all: driver

driver:
	@echo "KDIR = $(KDIR)"
	@echo "CROSS_COMPILE = $(CROSS_COMPILE)"
	@$(CHECK)
	@$(MAKE) -C $(KDIR) M=$(DRVDIR) modules

# Strip off debug symbols if not debugging (makes file size smaller)
strip:
	$(CROSS_COMPILE)strip --strip-debug sysfs_example.ko

clean:
	@$(MAKE) -C $(KDIR) M=$(DRVDIR) $@
	@-rm -f sysfs_example.ko

# Send the module to the targe board over serial console using ZMODEM
# Enter "rz -y" on the target board (from a R/W directory sush as /tmp), then
# run 'make send' to start the transfer.
# Requires package 'lrzsz' installed on both host and target
send:
	sz -b sysfs_example.ko > /dev/ttyACM0 < /dev/ttyACM0

endif

