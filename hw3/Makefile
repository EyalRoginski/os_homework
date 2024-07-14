# create object and kernel loadable module)
obj-m := message_slot.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CFLAGS=-std=c11
export CFLAGS


all:
	$(MAKE) -C $(KDIR) M=$(PWD) CFLAGS=$(CFLAGS) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
