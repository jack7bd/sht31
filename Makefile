DEVICE=sht31
KERNEL=/opt/rpi/linux
obj-m += ${DEVICE}.o

all:
	make ARCH=arm CROSS_COMPILE=/opt/rpi/bin/arm-linux-gnueabihf- -C $(KERNEL) M=$(shell pwd) modules

clean:
	make -C $(KERNEL) M=$(shell pwd) clean
