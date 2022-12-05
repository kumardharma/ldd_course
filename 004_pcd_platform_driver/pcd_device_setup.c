#include<linux/module.h>
#include<linux/platform_device.h>
#include "platform.h"

//#define pr_fmt
#undef pr_fmt

#define pr_fmt(fmt) "%s : " fmt,__func__

void pcdev_release(struct device *dev)
{//inour case there is nothing to release ,bcz our device  an array and static declaration

	pr_info("device released\n");
}


/*1.create platform data(i.e. device private data),we have 2 devices so use array*/

struct pcdev_platform_data pcdev_pdata[2] = {

	[0] = { .size = 512, .perm = RDWR, .serial_number = "PCDEVABC1111"},
	[1] = { .size = 1024, .perm = RDWR, .serial_number = "PCDEVXYZ2222"}

};
//2. create 2 platform devices using the below 

struct platform_device platform_pcdev_1 = {

	.name = "pseudo-char-device",
	.id = 0, //in group of devices,it is used as index

	.dev={
		.platform_data = &pcdev_pdata[0],
		.release = pcdev_release
	}

};

struct platform_device platform_pcdev_2 = {
	
	.name = "pseudo-char-device",
	.id = 1,

	.dev = {
		.platform_data = &pcdev_pdata[1],
		.release = pcdev_release

	}
};


static int __init pcdev_platform_init(void)
{
	/*register platform devices */

	platform_device_register(&platform_pcdev_1);//passing ptr to our device
	platform_device_register(&platform_pcdev_2);


	pr_info("Device setup module loaded \n");

	return 0;

}

static void __exit pcdev_platform_exit(void)
{

	/*whenever we unload the module, remove your 
	 platform devices*/

	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);

	pr_info("Device setup module unloaded \n");


}


module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("DHARMA_PRODUCTION");
MODULE_DESCRIPTION("Module which registers platform devices");
