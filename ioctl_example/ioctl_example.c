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
};

/* Global variables for this driver only */
static int major_num, minor_num;
static struct miscdevice my_dev;
static uint32_t my_value;

/* ioctl Commands */
/* The _IO macros must be used to create the ioctl command numbers.
 * For more info, see "Documentation/ioctl/ioctl-number.txt" in the kernel source tree
 * MACROS:
 *   _IO    an ioctl with no parameters
 *   _IOW   an ioctl with write parameters (copy_from_user)
 *   _IOR   an ioctl with read parameters  (copy_to_user)
 *   _IOWR  an ioctl with both write and read parameters.
 * ARGUMENTS:
 *   type - an 8-bit number defining the class (Documentation/ioctl/ioctl-number.txt)
 *   number - an 8-bit number used to define your custom commands (The combinations of 'type' and 'number' needs to be globally unique for your driver)
 *   data_type - what is the data that the ioclt is passing/returning (many times it's a struct)
 * 
 */
enum {
	IOCTL_CMD1 = _IOR(0xC1,0x20,uint32_t),
	IOCTL_CMD2 = _IOW(0xC1,0x21,uint32_t),
	IOCTL_CMD3 = _IOWR(0xC1,0x22,__u8[12])
};

static int drvsmpl_reverse_string( unsigned long __user arg )
{
	int i;
	u8 my_buffer_in[12];
	u8 my_buffer_out[12];
	u8 tmp;
	unsigned long not_copied;

	/* Copy out the data that was passed (we can't access it directly) */
	/* my_buffer_out[] <--- arg[] */
	not_copied = copy_from_user((void *)&my_buffer_in, (void *)arg, 12);
	if ( not_copied )
		return -EFAULT;

	/* Reverse the order of the ASCII string (don't assume a NULL) */
	for(i=0; i < 12; i++) {
		tmp = my_buffer_in[i];
		if( tmp == 0 )
			tmp = ' '; // replace NULLs with spaces
		my_buffer_out[11 - i] = tmp;
	}

	/* Pass our new data back */
	/* arg[] <--- my_buffer_out[] */
	not_copied = copy_to_user((void *)arg, (void *)&my_buffer_out, 12);
	if ( not_copied )
		return -EFAULT;

	return 0;
}

static int drvsmpl_open(struct inode * inode, struct file * filp)
{
	/* Dummy function (not used for this example) */
	return 0;
}

static int drvsmpl_release(struct inode * inode, struct file * filp)
{
	/* Dummy function (not used for this example) */
	return 0;
}

/* File IOCTL Operation */
static long drvsmpl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long not_copied;

	DPRINTK("%s called\n",__func__);

	switch ( cmd ) {
		case IOCTL_CMD1:
			/* Client wants to read 32-bit value */
			not_copied = copy_to_user((void *)arg, /* to */
				(void *)&my_value, /* from */
				sizeof(my_value) );
			
			if( not_copied )
				return -EFAULT;
			break;
		case IOCTL_CMD2:
			/* Client wants to write 32-bit value */
			not_copied = copy_from_user((void *)&my_value, /* to */
				(void *)arg, /* from */
				sizeof(my_value) );
			
			if( not_copied )
				return -EFAULT;
			break;
		case IOCTL_CMD3:
			/* Read in string, reverse it, send it back */
			ret = drvsmpl_reverse_string( arg );
			break;

		default:
			ret = -EINVAL;
	}

	return ret;
}


/* Define which file operations are supported */
/* The full list is in include/linux/fs.h */
struct file_operations drvsmpl_fops = {
	.owner		=	THIS_MODULE,
	.read		=	NULL,
	.write		=	NULL,
	.unlocked_ioctl	=	drvsmpl_ioctl,
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

