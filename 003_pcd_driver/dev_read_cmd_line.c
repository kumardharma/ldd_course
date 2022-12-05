#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>

char buffer[2048];

#define TRY_READ 10

int main(int argc, char *argv[])

//int main(void)
{
	int fd;
	
	/*this variable holds remaining data bytes to be read */
	int remaining ;
	
	/*Holds count of total data bytes read so far */
	int total_read=0;
	
	int n =0,ret=0;

	if(argc != 2 ){
		printf("Wrong usage\n");
		printf("Correct usage: <file> <readcount>\n");
		goto out;
	}

	remaining = atoi(argv[1]);
	printf("read requested = %d\n",remaining);


	/*convert command line supplied data to integer */
	//remaining = atoi(argv[1]); this for cmd lin args
	

	printf("read requested = %d\n",remaining);


	fd = open("/dev/pcdev-3",O_RDONLY);

	if(fd < 0){
		/*perror decodes user space errno variable and prints cause of failure*/
		perror("open");
		return fd;
		goto out;
	}

	printf("open was successful\n");

#if 1
	/*activate this for lseek testing */
	ret = lseek(fd,-10,SEEK_SET);
	if(ret < 0){
		perror("lseek");
		close(fd);
		goto close_fd;
	}
#endif
	/*Lets attempt reading twice */
	
	while(n != 2 && remaining)
	{
		/*read data from 'fd' */
		ret = read(fd,&buffer[total_read],remaining);

		if(!ret){
			/*There is nothing to read, i.e. read returned 0 */
			printf("end of file \n");
			break;
		}else if(ret <= remaining){//remaining is treated ac no of counts to be read
			printf("read %d bytes of data \n",ret );
			/*'ret' contains count of data bytes successfully read , so add it to 'total_read' */
		        total_read += ret;
			/*We read some data, so decrement 'remaining'*/
			remaining -= ret;
		}else if(ret < 0){
			printf("something went wrong\n");
			break;
		}else
			break;

		n++;
	}

	printf("total_read = %d\n",total_read);

	//dump buffer
	for(int i=0 ; i < total_read ; i++)
		printf("%c",buffer[i]);


close_fd:
	close(fd);
out:
	//close(fd);	
	return 0;
}
