#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <asm/io.h>

#define MY_WORK_QUEUE_NAME "WQsched.c"
#define DEVICE_NAME "sigrand_test_dev"

MODULE_AUTHOR("Vladimir O. Luchko <vlad.luch@mail.ru>");
MODULE_DESCRIPTION("Test task from Sigrand");
MODULE_LICENSE("GPL");

static struct workqueue_struct *my_workqueue;
static dev_t dev;
static int module_is_on;
static struct file_operations tst_dev_fops;

//-----------------------------------------------------------

static void got_char(void *scancode)
{
	static int win_pressed = 0;

	if(	((int)*((char *)scancode) & 0x7F) == 0x5b &&
  		!(*((char *)scancode) & 0x80) )
	{
		win_pressed = 1;
		return;
	}
	else if( ((int)*((char *)scancode) & 0x7F) == 0x5b &&
  		(*((char *)scancode) & 0x80) )
	{
		win_pressed = 0;	
		return;
	}

	if( win_pressed && // Win pressed and
		((int)*((char *)scancode) & 0x7F) == 0x20 && // d
  		!(*((char *)scancode) & 0x80) ) // pressed
	{
		char *argv[2] = { NULL, NULL };
		char *envp[3] = { NULL, NULL, NULL };

		argv[0] = "/sbin/test-script";

		envp[0] = "HOME=/";
		envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

		call_usermodehelper(argv[0], argv, envp, 0);
	}
};

//-----------------------------------------------------------

irqreturn_t irq_handler(int irq, void *dev_id)
{
	static int initialised = 0;
	static unsigned char scancode;
	static struct work_struct task;
	unsigned char status;

	if( !module_is_on )
	{
		return IRQ_HANDLED;
	}

	status = inb(0x64);
	scancode = inb(0x60);

	if (initialised == 0) {
		INIT_WORK(&task, got_char, &scancode);
		initialised = 1;
	} else {
		PREPARE_WORK(&task, got_char, &scancode);
	}

	queue_work(my_workqueue, &task);

	return IRQ_HANDLED;
};

//-----------------------------------------------------------

static int __init test_init(void)
{
	unsigned int major;
	//char *argv[6] = { NULL, NULL, NULL, NULL, NULL, NULL};
	//char *envp[3] = { NULL, NULL, NULL };
	my_workqueue = create_workqueue(MY_WORK_QUEUE_NAME);

	module_is_on = 0;

	if( request_irq(
			1,
			irq_handler,
			SA_SHIRQ,
			"test_keyboard_irq_handler",
			(void *)(irq_handler)) ){
		printk(KERN_INFO"TEST MODULE ERROR ON START\n");
		return 1;
		}
	major = register_chrdev(0, DEVICE_NAME, &tst_dev_fops);
	dev = MKDEV(major, 0);

	printk(KERN_INFO"TEST MODULE IS ON\n");
	printk(KERN_INFO"Major device number is %d\n", major);
/*
	argv[0] = "mknod";
	argv[1] = "/dev/test";
	argv[2] = "c";
	argv[3] = (char*)kmalloc(15, GFP_KERNEL);
	sprintf(argv[3],"%d", major);
	argv[4] = "0";

	envp[0] = "HOME=/";
	envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

	call_usermodehelper(argv[0], argv, envp, 0);
	//printk(KERN_INFO"%s %s %s %s %s\n",
		//argv[0],argv[1],argv[2],argv[3],argv[4]);
	kfree(argv[3]);
*/
	module_is_on = 1;
	return 0;
};

//-----------------------------------------------------------

static void __exit test_exit(void)
{
	/*
	char *argv[3] = { NULL, NULL, NULL };
	char *envp[3] = { NULL, NULL, NULL };

	argv[0] = "rm";
	argv[1] = "/dev/test";
	envp[0] = "HOME=/";
	envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

	call_usermodehelper(argv[0], argv, envp, 0);
*/
	unregister_chrdev(MAJOR(dev), DEVICE_NAME);
	free_irq(1, (void *)irq_handler );
	printk(KERN_INFO"TEST MODULE IS OFF\n");
	module_is_on = 0;
};

//-----------------------------------------------------------

static ssize_t
tst_dev_write(
	struct file * file, 
	const char __user * buffer,
	size_t count, 
	loff_t *ppos)
{
	unsigned char action;

	copy_from_user(&action, buffer, 1);	

	if(	action == '1' && !module_is_on){
		printk(KERN_INFO"TEST MODULE STARTED\n");
		module_is_on = 1;
		} 
	else if( action == '0' && module_is_on) {
		printk(KERN_INFO"TEST MODULE STOPPED\n");
		module_is_on = 0;
		}

	return 1;	
};

//-----------------------------------------------------------

static struct file_operations tst_dev_fops = {
	.write		= tst_dev_write
};

//-----------------------------------------------------------

module_init(test_init);
module_exit(test_exit);

