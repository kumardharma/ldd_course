#include "pcd_platform_driver_dt_sysfs.h"
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



loff_t pcd_lseek(struct file *filp,loff_t offset,int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->pdata.size;
	loff_t temp;

	pr_info("lseek requested\n");
	pr_info("Current value of file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset>max_size) || (offset  < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp>max_size) || (temp <0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default: return -EINVAL;

	}

	pr_info("New value of file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp,char __user *buff,size_t count,loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;

	pr_info("Read requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	/*adjust count*/
	if((*f_pos+count) > max_size)
		count = max_size - *f_pos;

	if(copy_to_user(buff,pcdev_data->buffer +(*f_pos),count)){
	
		return -EFAULT;
	}

	*f_pos += count;
	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);


	return count;
}




ssize_t pcd_write(struct file *filp,const char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;

	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);


	/*adjust the count*/
	if((*f_pos + count)> max_size)
		count = max_size - *f_pos;
	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}
	if(copy_from_user(pcdev_data->buffer +(*f_pos),buff,count)){
	
	
		return -ENOMEM;

	}
	/*update current file position */
	*f_pos += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file podition = %lld\n",*f_pos);


	return count;
}






int pcd_open(struct inode *inode,struct file *filp)
{
	int ret ;
	int minor_n;
	struct pcdev_private_data *pcdev_data;
	/*find out o which device file open was attempted bu the user space*/

	minor_n=MINOR(inode->i_rdev);
	pr_info(",inor acces = %d\n",minor_n);

	/*get dev private data structure*/
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	/*to supply device private to the other methods of the driver */
	filp->private_data = pcdev_data;

	/*check permission */
	ret = check_permission(pcdev_data->pdata.perm,filp->f_mode);
	(!ret)?pr_info("open was successful\n"):pr_info("open was un-successful\n");
	return ret;
}


int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release was successful\n");
	return 0;
}
