
#*obj-m :=pcd_n.o

obj-m :=add_more_dev_pcd_device_setup.o   kernel_rsource_API_pcd_platform_driver.o


#create some variables

ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR=/home/electro/udemy/workspace/ldd/source/linux_bbb_5.4/
HOST_KERN_DIR=/lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean


help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help


#to build against host instead of target

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules

	
#make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/electro/udemy/workspace/ldd/source/linux_bbb_4.14/ M=$PWD modules
