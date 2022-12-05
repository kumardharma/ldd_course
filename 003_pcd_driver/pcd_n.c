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


//#define DEV_MEM_SIZE 512

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

#define NO_OF_DEVICES 4

//create a small mem area to use it as a device
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];


//cdev variable

//struct cdev pcd_cdev;






/*some macros for permissions*/

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11





/*Device private data*/
struct pcdev_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};



/*driver's private data structure*/

struct pcdrv_private_data
{
	int total_devices;
	/*keep dev no here bcz driver need dev no not the device*/

	dev_t device_number;/*holds dev num*/
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];

	struct class *class_pcd;
	struct device *device_pcd;
};

/* this holds device number*/


/*cdev variable*/
//struct cdev pcd_cdev;/*no need bcz cdev is needed per device so it in pcdev_private dATA


struct pcdrv_private_data pcdrv_data =
{
	.total_devices=NO_OF_DEVICES,

	/*class_pcd and device_pcd will be initialized using kernel API,not here*/
	//initialize all 4 devices

	.pcdev_data = {

		[0] = {
			.buffer = device_buffer_pcdev1,
			.size =MEM_SIZE_MAX_PCDEV1,
			.serial_number ="PCDEV1XYZ123",
			.perm = RDONLY /*RDONLY*/
				/*for cdev we have API ,noyt initia here*/
		},

		[1] = {
			.buffer = device_buffer_pcdev2,
			.size =MEM_SIZE_MAX_PCDEV2,
			.serial_number ="PCDEV2XYZ123",
			.perm = WRONLY /*WRONLY*/
				/*for cdev we have API ,noyt initia here*/
		},

		[2] = {
			.buffer = device_buffer_pcdev3,
			.size =MEM_SIZE_MAX_PCDEV3,
			.serial_number ="PCDEV3XYZ123",
			.perm = RDWR /*RDWR*/
				/*for cdev we have API ,noyt initia here*/
		},

		[3] = {
			.buffer = device_buffer_pcdev4,
			.size =MEM_SIZE_MAX_PCDEV4,
			.serial_number ="PCDEV4XYZ123",
			.perm = 0x11 /*RDWR*/
				/*for cdev we have API ,noyt initia here*/
		}
	}

};

/////////////////////////////////////////methods of driver befor file opration

//all methods of driver(read,write..etc.

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{


	/*create variable for dev priv data,wecan extract it from
         *"struct file *filp" in this str one menber there "(void*)private_data" use this variable 
	 to get device private data also*/

        struct pcdev_private_data *pcdev_data =(struct pcdev_private_data*)filp->private_data;

        int max_size=pcdev_data->size;


	loff_t temp;
	pr_info("lseek requested\n");
	pr_info("current value of file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset>max_size) || (offset < 0))
				return -EINVAL;//seeking beyond device,dont
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp =filp->f_pos + offset;

			/*first calc final val of file operation of file 
			  position by adding oofset to that*/
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;//otherwise take that value
			break;
		case SEEK_END:
			temp= max_size + offset;

			if((temp > max_size) || (temp < 0))
			{
				/*if cond true then dont take that
				  whence val,otherwise we can take that value i.e. temp*/
				return -EINVAL;
			}
			filp->f_pos=temp;

			filp->f_pos = max_size + offset;
			break;
		default:
			return -EINVAL;
	}
	pr_info("new value of the file position = %lld\n",filp->f_pos);
	return filp->f_pos;//return usdated fil pos


}
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{

	/*create variable for dev priv data,wecan extract it from 
	 *"struct file *filp" in this str one menber there "(void*)private_data" use this variable tu get device private data also*/

	struct pcdev_private_data *pcdev_data =(struct pcdev_private_data*)filp->private_data;

	int max_size=pcdev_data->size;


	pr_info("read requested for %zu value\n",count);
	pr_info("current file position = %lld\n",*f_pos);

	/*Adjust the 'count' */

	if((*f_pos + count) > max_size)
	{
		count=max_size - *f_pos;
	}

	/*copy to user*/

	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count))
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


	/*create variable for dev priv data,wecan extract it from
	 *"struct file *filp" in this str one menber there "(void*)private_data" use this variable tu get device private data also*/

	struct pcdev_private_data *pcdev_data =(struct pcdev_private_data*)filp->private_data;

	int max_size=pcdev_data->size;


	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	/*adjust the 'count'*/
	if((*f_pos+count) > max_size)
		count=max_size - *f_pos;
	if(!count)
	{
		pr_err("No space left on the device\n");
		return -ENOMEM;
	}
	/* copy from user*/

	if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count))
		return -EFAULT;

	/*update the current file position*/

	*f_pos +=count;

	pr_info("Number of bytes successfully written= %zu\n",count);
	pr_info("Updated file position =%lld\n",*f_pos);

	/*return no of bytes written successfully*/
	return count;
	
}

int check_permission(int dev_perm, int acc_mode)
{
	if(dev_perm == RDWR)//for read write no need to check
		return 0;
	if((dev_perm == RDONLY) && ( (acc_mode & FMODE_READ)  && !(acc_mode  & FMODE_WRITE)))
	/*if device perm is read only- and- if file is opend for read -and- 
	 not opend for write ,then return success that is 0*/
		return 0;

	/*write only access*/
	if((dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE)  && !(acc_mode  & FMODE_READ)))
		return 0;

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	int minor_n;

	struct pcdev_private_data *pcdev_data;

	/*find out on whhich device file open was attempted by the user space*/

	minor_n=MINOR(inode->i_rdev);//inode member gave dev no
	pr_info("minor access = %d\n",minor_n);

	/*get the device's private data structure:-access by using a member *i_cdev,
	  it gives address of the str whos member is cdev(that is our 
	  pcdev_private_data str cdev is member here */

	pcdev_data =container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	/*this data str is req for all other methods read/write/lseek so 
	  we have to save it:-how:-once the deviced is opened, "struct file *filp"
	  kernel object is created in this str there is a member "*private_data" 
	  is used tom pass this kernel object to other methods*/

	/*passing device private data to the other methods of the driver */
	filp->private_data = pcdev_data;

	/*Because, for example, in our case, the pcdev1 device is read only.
	  That's why, the user application shoul not try to open the device file
	  of pcdev1 with read-write access.So, the driver should detect that and 
	  it should then report whether the open was successful or not.
	  That's why, the permission checking should be done in the open method.*/


		/*returns error code or zero on failure,aguments permission field
		 and f_mode field of "struct file" */

	ret = check_permission(pcdev_data->perm,filp->f_mode);

	(!ret)?pr_info("open was successful\n"):pr_info("open was unsuccessful\n"); 






	//	pr_info("Open was successful\n");
	return ret;
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

//move to driver,s prv data
/*struct class *class_pcd;
  struct device *device_pcd;//5th in __init fn*/

static int __init pcd_driver_init(void)
{

	int ret;
	int i;

	/*1.dynmclly allo dev number*/
	ret=alloc_chrdev_region(&pcdrv_data.device_number,0,4,"pcd_devices");

	/*this fn may fail and returns  0 or -ve no when it fails, so add goto statements* create one variable "int ret" to catch return val*/


	if(ret<0)
	{
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	//print major and minor numbers,__func__ is to know which fn it got printed,here fn is 
	//__init pcd_driver_init(void)

	/*4.Create device class under /sys/class/,declare var(ptr) to catch ret value,  */

	pcdrv_data.class_pcd=class_create(THIS_MODULE,"pcd_class");/*it ret a ptr which
								     may be valid or not,we'll see error handling later*/
	/*use the retuned pointer to check the error in goto,it return valid ptr on succ and ERR_PTR() on error*/
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_info("Class creation failed\n");
		ret=PTR_ERR(pcdrv_data.class_pcd);//convert ptr into err code(int)
		goto cdev_del;
	}
	for(i=0;i<NO_OF_DEVICES;i++){
		pr_info("Deice number <major>:minor = %d:%d",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));

		/*2. initialize the cdev structure with fops*/
		cdev_init(&pcdrv_data.pcdev_data[i].cdev,&pcd_fops);/*it ret is void, need not check in goto*/

		/*3.register device(cdev structure) with VFS*/
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret=cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);//ret 0 on succ, -ve on fail
		if(ret<0)
		{
			pr_err("Cdev add failed\n");
			goto unreg_chrdev;//level name
		}


		/*4.Create device class under /sys/class/,declare var(ptr) to catch ret value,  */

		/*class_pcd=class_create(THIS_MODULE,"pcd_class");*it ret a ptr which
		  may be valid or not,we'll see error handling later*/
		/*use the retuned pointer to check the error in goto,it return valid ptr on succ and ERR_PTR() on error
		  if(IS_ERR(class_pcd))
		  {
		  pr_info("Class creation failed\n");
		  ret=PTR_ERR(class_pcd);//convert ptr into err code(int)
		  goto cdev_del; 
		  }*/





		/*5. device file creation i.e. populate the sysfs with device inf */

		pcdrv_data.device_pcd=device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number+i,NULL,"pcdev-%d",i+1);//this name will appear in dev 
		//directory,this returns a ptr to struct device,so dec a var of this type
		//and this ptr is required when we have to destroy the device so catch it

		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_err("Device create failed\n");
			ret=PTR_ERR(pcdrv_data.device_pcd);
			goto class_del;
		}
	}

	pr_info("Module init was successful\n");


	return 0;
	//all goto labels

class_del:
	class_destroy(pcdrv_data.class_pcd);
cdev_del:
	for(;i>=0;i--){
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
	//cdev_del(&pcdrv_data.pcdev_data[i].cdev);
unreg_chrdev://here undo prev opern i.e. unreg alloc_chrdev_reg()
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
	pr_info("Module insertion failed\n");
	return ret;

}
static void __exit pcd_driver_exit(void)
{

	int i;
	for(i=0;i<NO_OF_DEVICES;i++){
		device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
	unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);



	pr_info("module unloaded\n");



#if 0
	device_destroy(class_pcd,device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number,1);
	pr_info("module unloaded\n");
#endif
}
module_init(pcd_driver_init);
module_exit(pcd_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DHARMA");
MODULE_DESCRIPTION("A simple pseudo driver which handles n devices");
