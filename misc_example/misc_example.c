/*
 * Driver Sample for RZ/A1
 * 
 * For your own driver, first do a text search-and-replace 
 * on the string "drvsmpl" everywhere in this file.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>		/* for struct file_operations */
#include <linux/errno.h>	/* Error codes */
#include <linux/slab.h>		/* for kzalloc */
#include <linux/device.h>	/* for sysfs interface */
#include <linux/mm.h>
#include <linux/interrupt.h>	/* for requesting interrupts */
#include <linux/sched.h>
#include <linux/ctype.h> 
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/miscdevice.h>	/* Misc Driver */

/* Options */
//#define DEBUG

#define DRIVER_VERSION	"2016-07-08"
#define DRIVER_NAME	"drvsmpl"

#ifdef DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, DRIVER_NAME, ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/* Our Private Data Structure */
struct drvsmpl {
	int	my_value;
	void * __iomem *reg;
	char message[50];
};

/* Global variables for this driver only */
static int major_num, minor_num;
static struct drvsmpl my_drvsmpl = {
		.message = "This is my message\n",
	};
static struct miscdevice my_dev;


static int drvsmpl_open(struct inode * inode, struct file * filp)
{
	/* Called when someone opens a handle to this device */
	DPRINTK("%s called\n",__func__);

	return 0;
}

static int drvsmpl_release(struct inode * inode, struct file * filp)
{
	/* Called when someone closes a handle to this device */
	DPRINTK("%s called\n",__func__);

	return 0;
}

/* File Read Operation */
/*

buf: The buffer to fill with you data
length: The size of the buffer that you need to fill

ppos: a pointer to the 'position' index that the file structure is using to know
	where in the file you are. You need to update this to show that you are
	moving your way through the file.

return: How many bytes of the buffer you filled. Note, the userland app might keep
	asking for more data until you return 0.
*/
static ssize_t drvsmpl_read(struct file * file, char __user * buf, size_t length, loff_t *ppos)
{
	int err;
	int copy_amount;

	DPRINTK("%s called. length=%d, *ppos=%ld\n",__func__, length, (long int)*ppos);

	copy_amount = strlen(my_drvsmpl.message);

	/* You check *ppos to see if they are asking for more data than you
	   have to send them */
	/* for example, we have 20 bytes of data, but they are asking for the 21st byte */
	if( *ppos >= copy_amount )
		return 0;	/* Return an End of File (no more data to send) */

	/* check if the passed buffer is smaller than the data we have to send */
	/* remember, *ppos is the starting point of the data to send, which might not
	   be 0 */
	if ( (copy_amount - *ppos) >= length )
		copy_amount = length;	// only copy what we can fit in the buffer
	else
		copy_amount = copy_amount - *ppos; // copy the reset

	err = copy_to_user(buf, my_drvsmpl.message + *ppos, copy_amount);
	if (err != 0)
		return -EFAULT;

	/* You need to update the pointer that was passed so
	   they will know how many bytes you copied into the buffer */
	*ppos += copy_amount;

	return copy_amount;
}

/* File Write Operation */
/*

buf: The buffer that contains data from the user (NOTE you cannot access it directly)

count: The amount of data in 'buf'

ppos: a pointer to the 'position' index that the file structure is using to know
	where in the file you are. You need to update this to show that you are
	moving your way through the file.

return: How many bytes of the buffer you read out. If you return a number less than
     'count', then the application might think it needs to send the same data again.
*/
static ssize_t drvsmpl_write(struct file * file, const char __user * buf,  size_t count, loff_t *ppos)
{
	int err;
	int i;
	int copy_ammount;
	char my_buffer[16];
	int  index = 0;

	DPRINTK("%s called. count=%d, *ppos=%ld\n",__func__, count, (long int)*ppos);

	/* zero out our local message buffer */
	memset( my_drvsmpl.message,0, sizeof(my_drvsmpl.message) );

	/* Copy data from user buffer to kernel buffer */
	/* Even though the user space  passed a buffer pointer, this driver
	 * cannot access that data directly. We must first copy it into
	 * a local buffer in kernel space */
	while ( index < count )
	{
		copy_ammount = count - index;
		if( copy_ammount > sizeof(my_buffer) )
			copy_ammount = sizeof(my_buffer);

		err = copy_from_user( my_buffer, buf + index, copy_ammount );
		if (err != 0)
			return -EFAULT;

		/* copy what was passed into our local message buffer */
		for( i=0 ; i < copy_ammount; i++ )
		{
			if( index < sizeof(my_drvsmpl.message) )
				my_drvsmpl.message[ index ] = my_buffer[i];
			index++;
		}

	}

#if 0
	/* Another way of doing same thing as above. */
	/* Allocate enough 'kernel' memory to hold all the incoming 'user' data and
	 * then have it all copied at once. The memdup_user function does all this for you. */
	{
		u8 *in_buf;
	
		in_buf = memdup_user(buf, count);
		if(IS_ERR(in_buf))
			return PTR_ERR(in_buf);

		/* Now only copy in as much as our local buffer can hold */
		if ( count >= sizeof(my_buffer) )
			copy_ammount = sizeof(my_buffer);
		else
			copy_ammount = count;

		memcpy(my_drvsmpl.message, in_buf, copy_ammount);
		kfree(in_buf);	/* free the memory allocated from memdup_user() */
	}
#endif

	/* You need to update the position pointer that was passed so
	   they will know how many bytes you copied into the buffer. If
	   the amount copied was less than the size of the buffer they
	   gave you, this function will be called again with your
	   updated ppos value so you can keep track of where you left off. */
	*ppos += count;

	return count;
}

/* Define which file operations are supported */
/* The full list is in include/linux/fs.h */
struct file_operations drvsmpl_fops = {
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	drvsmpl_read,
	.write		=	drvsmpl_write,
	.unlocked_ioctl	=	NULL,
	.open		=	drvsmpl_open,
	.release	=	drvsmpl_release,
};

/* This 'init' function of the driver will run when the system is booting.
   It will only run once no matter how many 'devices' this driver will
   control. Its main job to register the driver interface (file I/O).
   Only after the kernel finds a 'device' that needs to use this driver
   will the 'probe' function be called.
   A 'device' is registered in the system after a platform_device_register()
   in your board-xxxx.c is called.
   If this is a simple driver that will only ever control 1 device,
	   you can do the device registration in the init routine and avoid having
   to edit your board-xxx.c file.

  For more info, read:
	https://www.kernel.org/doc/Documentation/driver-model/platform.txt
*/
static int __init drvsmpl_init(void)
{
	int ret;

	DPRINTK("%s called\n",__func__);

	my_dev.minor = MISC_DYNAMIC_MINOR;
	my_dev.name = "drvsmpl";	// will show up as /dev/drvsmpl
	my_dev.fops = &drvsmpl_fops;
	ret = misc_register(&my_dev);
	if (ret) {
		printk(KERN_ERR "Misc Device Registration Failed\n");
		return ret;
	}

	/* Save the major/minor number that was assigned */
	major_num = MISC_MAJOR;	/* same for all misc devices */
	minor_num = my_dev.minor;

	printk("%s version %s\n", DRIVER_NAME, DRIVER_VERSION);

	return ret;
}

static void __exit drvsmpl_exit(void)
{
	DPRINTK("%s called\n",__func__);

	misc_deregister(&my_dev);
}

module_init(drvsmpl_init);
module_exit(drvsmpl_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chris Brandt");
MODULE_ALIAS("platform:drvsmpl");
MODULE_DESCRIPTION("drvsmpl: Example of a simple Linux driver");

