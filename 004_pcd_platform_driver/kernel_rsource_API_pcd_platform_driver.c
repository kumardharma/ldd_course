#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>
#include<linux/slab.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

/*Device private data structure */
struct pcdev_private_data
{
	struct pcdev_platform_data pdata;
	char *buffer;
	dev_t dev_num;
	struct cdev cdev;
};

/*Device private data structure */
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_num_base;
	struct class *class_pcd;
	struct device *device_pcd;
};

/* create a global var for driver pvt data, but we'll alloc mem 
   dynmcly for dev prvt data when dev detected */

struct  pcdrv_private_data pcdrv_data;

int check_permission(int dev_perm,int acc_mode)

{
	if(dev_perm == RDWR)
		return 0;
	//ensure read only access
	if((dev_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;

	if((dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;
	return -EPERM;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{


	return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
{

	return 0;
}

ssize_t pcd_write(struct file *filp,const  char __user *buff, size_t count, loff_t *fpos)
{

	return -ENOMEM;
}
int pcd_open(struct inode *inode, struct file *filep)
{
	pr_info("open was successful\n");
	return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release was successful\n");

	return 0;
}



/*file operations of the driver */

struct file_operations pcd_fops=
{
	.open = pcd_open,
	.release = pcd_release,
	.read =pcd_read,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.owner = THIS_MODULE
};



/*gets called when the device is removed frmm the system*/
int pcd_platform_driver_remove(struct platform_device *pdev)
{
	struct pcdev_private_data *dev_data= dev_get_drvdata(&pdev->dev);/*argument is 
									"struct device *dev" type, in
									this str "driver_data" is there,
									using this "dev_data" get the
									device number*/

	/*1. Remove a device that was created with device create()*/	 
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);

	/*Rmove a cdev entry from the system*/
	cdev_del(&dev_data->cdev);

	/*3. Free the memory held by the device */
	/*kfree(dev_data->buffer);
	kfree(dev_data); sice we used devm_kzalloc() no need to free,kernel will do*/

	pcdrv_data.total_devices--;

	pr_info("A device is removed\n");	 
	return 0;
}

/*called when mached with some device name*/
int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;
	struct pcdev_private_data *dev_data;//for getting dev prvt data
	struct pcdev_platform_data *pdata;//to get platfrm data


	pr_info("A device is detected\n");

	/*1. Get the pltfrm data*/
	pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);//typecasted
							//bcz ret type is void*
	
	if(!pdata)
	{
		pr_info("No platform data available\n");
		ret = -EINVAL;
		goto out;
	}



	/*2. Dynamically allocate memory for the device private data */
	//dev_data = kzalloc(sizeof(struct pcdev_private_data),GFP_KERNEL);
	dev_data = devm_kzalloc(&pdev->dev,sizeof(struct pcdev_private_data),GFP_KERNEL);
		/*this memory allocationis being done on behalf of device dev(struct device type),
		in remove fn no need to free, but in probe fn devm_kfree() should be there 
		to the cases where probe fails*/
	//check for null
	
	if(!dev_data)
	{
		pr_info("Cannot allocate memory \n");
		ret = -ENOMEM;
		goto out;
	}//if mem alloc  is succ then cpy data from pltfrm dati


	pdev->dev.driver_data = dev_data;/*saving the device private data ptr ,in platform_device str
					   there is "struct device dev",and in this str there is 
					   "*driver_data", to access device_private_data in other fns
					   using the local variable "dev_data" */ 
	/*or we can use fn:"dev_set_drvdata(*pdev->,dev_data);" */

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
	pr_info("Device size = %d\n",dev_data->pdata.size);
	pr_info("Device permission = %d\n",dev_data->pdata.perm);




	/*3. Dynamically allocate memory for the device buffer using size 
	 info from the platform data */
	dev_data->buffer =devm_kzalloc(&pdev->dev,dev_data->pdata.size,GFP_KERNEL);
        //check for null

        if(!dev_data->buffer)	 
	{
                pr_info("Cannot allocate memory \n");
                ret = -ENOMEM;
		//and free the mem allc in step 2 using the goto label
                goto dev_data_free;
	}


	/*4. Get the device number, from driver data structure */
	dev_data->dev_num = pcdrv_data.device_num_base + pdev->id;
	//this id is imp from "struct platform_device *pdev",argument of prob fn
	


	/*5.Do cdev init and cdev add */
	cdev_init(&dev_data->cdev,&pcd_fops);
	
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret < 0)
	{
		pr_err("Cdev add failed \n");
		//so clean the mem buffer
		goto buffer_free;
	}


	/*6. Create device file for the detected platfrm device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,dev_data->dev_num,NULL,"pcdev-%d",pdev->id);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("Device create failed\n");
		goto cdev_del;
	}



	pr_info("the probe was successful\n");
	return 0;


	pcdrv_data.total_devices++;

/*7 error handling,using labels*/

cdev_del:
	cdev_del(&dev_data->cdev);

buffer_free:
	devm_kfree(&pdev->dev,dev_data->buffer);
dev_data_free:
	devm_kfree(&pdev->dev,dev_data);

out:
	pr_info("Device probe failed\n");
	return ret;

}

struct platform_driver pcd_platform_driver =
{
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver = {
		//member of "struct device_driver driver"

		.name = "pseudo-char-device"
	}
};

#define MAX_DEVICES 10



static int __init pcd_platform_driver_init(void)
{
	int ret;
	/*1 Dynamically allocate a device num for MAX_DEVICES declared above ,
	 first argumnt is a var add of dev_t type present in driver prvt data structure*/
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");
	if(ret<0)
	{
		pr_err("Alloc chrdev  failed\n");
		return ret;
	}

	/*2. Create device class under /sys/class */
	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed\n");

		ret=PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
		return ret;
	}

	/*3. rgister the platform driver*/
	platform_driver_register(&pcd_platform_driver);
	pr_info("pcd platform driver loaded\n");
	return 0;
}
static void __exit pcd_platform_driver_cleanup(void)
{
	/*1. Unregister platform driver */
	platform_driver_unregister(&pcd_platform_driver);

	/*2. Class destroy*/
	class_destroy(pcdrv_data.class_pcd);
	
	/*3. Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);

	pr_info("pcd platform deriver unloaded\n");


}
module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DHARMA_PRODUCTION");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n pc pcdevs");
