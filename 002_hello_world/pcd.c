#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__ 

/*macro to print fn name which fn is printing(instead of __func__)
  we have concatenated %s and fmt,remove fom pr_info()
  pr_info("%s :Deice number <major>:minor = %d:%d",__func__,MAJOR(device_number),MINOR(device_number)); */


#define DEV_MEM_SIZE 512
//create a small mem area to use it as a device
char device_buffer[DEV_MEM_SIZE];

dev_t device_number;/*holds dev num*/

//cdev variable

struct cdev pcd_cdev;

/////////////////////////////////////////methods of driver befor file opration

//all methods of driver(read,write..etc.

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	loff_t temp;
	pr_info("lseek requested\n");
	pr_info("current value of file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset>DEV_MEM_SIZE) || (offset < 0))
				return -EINVAL;//seeking beyond device,dont
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp =filp->f_pos + offset;

			/*first calc final val of file operation of file 
			  position by adding oofset to that*/
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;//otherwise take that value
			break;
		case SEEK_END:
			temp= DEV_MEM_SIZE + offset;

			if((temp > DEV_MEM_SIZE) || (temp < 0))
			{
				/*if cond true then dont take that
				  whence val,otherwise we can take that value i.e. temp*/
				return -EINVAL;
			}
			filp->f_pos=temp;

			filp->f_pos = DEV_MEM_SIZE + offset;
			break;
		default:
			return -EINVAL;
	}
	pr_info("new value of the file position = %lld\n",filp->f_pos);
	return filp->f_pos;//return usdated fil pos
}
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested for %zu value\n",count);
	pr_info("current file position = %lld\n",*f_pos);

	/*Adjust the 'count' */

	if((*f_pos + count) > DEV_MEM_SIZE)
	{
		count=DEV_MEM_SIZE - *f_pos;
	}

	/*copy to user*/

	if(copy_to_user(buff,&device_buffer[*f_pos],count))
	{
		return -EFAULT;
	}

	/*update the current file position */

	*f_pos += count;

	pr_info("Number of bytes successfully read= %zu\n",count);
	pr_info("Updated file position =%lld\n",*f_pos);

	/*return the current file position */

	return count;
}
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	/*adjust the 'count'*/
	if((*f_pos+count) > DEV_MEM_SIZE)
		count=DEV_MEM_SIZE - *f_pos;
	if(!count)
	{
		pr_err("No space left on the device\n");
		return -ENOMEM;
	}
	/* copy from user*/

	if(copy_from_user(&device_buffer[*f_pos],buff,count))
		return -EFAULT;

	/*update the current file position*/

	*f_pos +=count;

	pr_info("Number of bytes successfully written= %zu\n",count);
	pr_info("Updated file position =%lld\n",*f_pos);

	/*return no of bytes written successfully*/
	return count;
}
int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("Open was successful\n");
	return 0;
}
int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("Release was successful\n");
	return 0;
}              

/////////////////////////////////////////////////////////////

//file operation of driver

struct file_operations pcd_fops=
{
	.open=pcd_open,
	.write=pcd_write,
	.read=pcd_read,
	.llseek=pcd_lseek,
	.release=pcd_release,
	.owner=THIS_MODULE,
};


struct class *class_pcd;
struct device *device_pcd;//5th in __init fn

static int __init pcd_driver_init(void)
{
	int ret;

	/*1.dynmclly allo dev number*/
	ret=alloc_chrdev_region(&device_number,0,1,"pcd_devices");

	/*this fn may fail and returns  0 or -ve no when it fails, so add goto statements* create one variable "int ret" to catch return val*/


	if(ret<0)
	{
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	//print major and minor numbers,__func__ is to know which fn it got printed,here fn is 
	//__init pcd_driver_init(void)

	pr_info("Deice number <major>:minor = %d:%d",MAJOR(device_number),MINOR(device_number));

	/*2. initialize the cdev structure with fops*/
	cdev_init(&pcd_cdev,&pcd_fops);/*it ret is void, need not check in goto*/

	/*3.register device(cdev structure) with VFS*/
	pcd_cdev.owner = THIS_MODULE;
	ret=cdev_add(&pcd_cdev,device_number,1);//ret 0 on succ, -ve on fail
	if(ret<0)
	{
		pr_err("Cdev add failed\n");
		goto unreg_chrdev;//level name
	}
	/*4.Create device class under /sys/class/,declare var(ptr) to catch ret value,  */

	class_pcd=class_create(THIS_MODULE,"pcd_class");/*it ret a ptr which
	may be valid or not,we'll see error handling later*/
	/*use the retuned pointer to check the error in goto,it return valid ptr on succ and ERR_PTR() on error*/    
	if(IS_ERR(class_pcd))
	{
		pr_info("Class creation failed\n");
		ret=PTR_ERR(class_pcd);//convert ptr into err code(int)
		goto cdev_del; 
	}
	/*5. device file creation i.e. populate the sysfs with device inf */

	device_pcd=device_create(class_pcd,NULL,device_number,NULL,"pcd");//this name will appear in dev 
	//directory,this returns a ptr to struct device,so dec a var of this type
	//and this ptr is required when we have to destroy the device so catch it

	if(IS_ERR(device_pcd))
	{
		pr_err("Device create failed\n");
		ret=PTR_ERR(device_pcd);
		goto class_del;
	}
	pr_info("Module init was successful\n");

	return 0;
	//all goto labels
	
class_del:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chrdev://here undo prev opern i.e. unreg alloc_chrdev_reg()
	unregister_chrdev_region(device_number,1);
out:
	pr_info("Module insertion failed\n");
	return ret;
}
static void __exit pcd_driver_exit(void)
{

	device_destroy(class_pcd,device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number,1);
	pr_info("module unloaded\n");

}
module_init(pcd_driver_init);
module_exit(pcd_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DHARMA");
MODULE_DESCRIPTION("A simple pseudo driver code");
