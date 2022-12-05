#include<linux/module.h>

static int __init hello_world_init(void)
{
	pr_info("HEllo World\n");
	return 0;
	
	
	//mod initialisation fn must return a value, load will be successful only when
	//init entry point func returns 0, if it return non-zero value for any reason  module
	//load will fail

}
static void __exit helloworld_cleanup(void)
{
	pr_info("Good bye World\n");
	
}
module_init(hello_world_init);
module_exit(helloworld_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dharma");
MODULE_DESCRIPTION("A hello world kernel module");
MODULE_INFO(board,"Beaglebone black REV A5");
