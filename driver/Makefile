DRIVER := hov_drv
GROUP  := gem5

ifneq ($(KERNELRELEASE),)
obj-m += $(DRIVER).o
ccflags-y += -I$(src)/../include

else
PWD         ?= $(shell pwd)
KVERSION    ?= $(shell uname -r)
KMAKEDIR    ?= /lib/modules/$(KVERSION)/build
MODLOADCONF ?= /etc/modules-load.d/$(GROUP).conf

.PHONY: build
build:
	make -C $(KMAKEDIR) M=$(PWD) modules

.PHONY: install
install:
	make -C $(KMAKEDIR) M=$(PWD) INSTALL_MOD_DIR=$(GROUP) modules_install

.PHONY: clean
clean:
	make -C $(KMAKEDIR) M=$(PWD) clean

endif
