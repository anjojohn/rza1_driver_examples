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
	/* Dummy function (not used for this example) */
	return 0;
}

static int drvsmpl_release(struct inode * inode, struct file * filp)
{
	/* Dummy function (not used for this example) */
	return 0;
}

/* File Read Operation */
static ssize_t drvsmpl_read(struct file * file, char __user * buf, size_t length, loff_t *ppos)
{
	/* Dummy function (not used for this example) */
	return 0;
}

/* File Write Operation */
static ssize_t drvsmpl_write(struct file * file, const char __user * buf,  size_t count, loff_t *ppos)
{
	/* Dummy function (not used for this example) */
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

/************************* sysfs attribute files ************************/
/* For a misc driver, these sysfs files will show up under:
	/sys/devices/virtual/misc/drvsmpl/
*/
static ssize_t drvsmpl_show_my_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
#if 1
	struct drvsmpl *pdata = &my_drvsmpl;
#else
	/* You would use this for 'platform' drivers where 1 driver controls
	 * multiple devices/channels (not a 'misc' driver that only controls
	 * one device)*/
	struct drvsmpl *pdata = dev_get_drvdata(dev);
#endif
	int count;

	count = sprintf(buf, "%d\n", pdata->my_value);

	/* Return the number characters (bytes) copied to the buffer */
	return count;
}

static ssize_t drvsmpl_store_my_value(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
#if 1
	struct drvsmpl *pdata = &my_drvsmpl;
#else
	/* You would use this for 'platform' drivers where 1 driver controls
	 * multiple devices/channels (not a 'misc' driver that only controls
	 * one device)*/
	struct drvsmpl *pdata = dev_get_drvdata(dev);
#endif

	/* Scan in our argument(s) */
	sscanf(buf, "%d", &pdata->my_value);

	/* Return the number of characters (bytes) we used from the buffer */
	return count;
}

static ssize_t drvsmpl_show_minor_num(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* Return the number characters (bytes) copied to the buffer */
	return sprintf(buf,"%d\n", minor_num);
}


static struct device_attribute drvsmpl_device_attributes[] = {
	__ATTR(	my_value, 			/* the name of the virtual file will appear as */
		0666, 				/* Virtual file permissions */
		drvsmpl_show_my_value, 		/* file read handler */
		drvsmpl_store_my_value), 	/* file write handler */
	__ATTR(major_num, 0444, drvsmpl_show_minor_num, NULL),
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
	int i;

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

	/* Add our sysfs interface virtual files to the system */
	/* NOTE: The device_create_file() feature can only be used with GPL drivers */
	for (i = 0; i < ARRAY_SIZE(drvsmpl_device_attributes); i++) {
		ret = device_create_file(my_dev.this_device, &drvsmpl_device_attributes[i]);
		if (ret < 0) {
			printk(KERN_ERR "device_create_file error\n");
			break;
		}
	}

	printk("%s version %s\n", DRIVER_NAME, DRIVER_VERSION);

	return ret;
}

static void __exit drvsmpl_exit(void)
{
	int i;

	DPRINTK("%s called\n",__func__);

	/* Remove our sysfs files */
	for (i = 0; i < ARRAY_SIZE(drvsmpl_device_attributes); i++)
		device_remove_file(my_dev.this_device, &drvsmpl_device_attributes[i]);
		
	/* Remove our driver from the system */
	misc_deregister(&my_dev);
}

module_init(drvsmpl_init);
module_exit(drvsmpl_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chris Brandt");
MODULE_ALIAS("platform:drvsmpl");
MODULE_DESCRIPTION("drvsmpl: Example of a simple Linux driver");

