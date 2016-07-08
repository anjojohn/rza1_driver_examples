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
#include <linux/proc_fs.h>	/* for procfs */

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

	struct proc_dir_entry *root_entry;
	struct proc_dir_entry *test1_entry;
};

/* Global variables for this driver only */
static struct drvsmpl my_drvsmpl;

/* File Read Operation */
static ssize_t drvsmpl_procfs1_read(struct file *file, char __user *buf,
					size_t length, loff_t *ppos)
{
	char my_buffer[20];
	int count;

	DPRINTK("%s called. length=%d, *ppos=%ld\n",__func__, length, (long)(*ppos & 0xFFFFFFFF));
	
	/* Only respond if asking for the begingin of the file */
	if( *ppos != 0 )
		return 0;	/* No more data....please stop asking */

	/* Convert to ASCII */
	count = sprintf(my_buffer, "%d\n", my_drvsmpl.my_value);

	/* copy into user buffer */
	if( copy_to_user(buf, my_buffer, count) != 0)
		return -EFAULT; /* Not all the data was copied */

	/* You need to update the pointer that was passed so
	   they will know how many bytes you copied into the buffer */
	*ppos += count;

	/* Return the number characters (bytes) copied to the buffer */
	return count;
}

/* File Write Operation */
static ssize_t drvsmpl_procfs1_write(struct file *file, const char __user *buf,
					size_t count, loff_t *ppos)
{
	char my_buffer[20];
	int ret;

	DPRINTK("%s called. count=%d, *ppos=%ld\n",__func__, count, (long)(*ppos & 0xFFFFFFFF));

	if( count > sizeof(my_buffer) )
		return -EFAULT;	/* they are sending more data then we expect */
	
	/* Get the data */
	ret = copy_from_user( my_buffer, buf, count );
	if (ret < 0)
		return -EFAULT;

	/* convert from ASCII(decimal) */
	sscanf(buf, "%d", &my_drvsmpl.my_value);

	/* You need to update the position pointer that was passed so
	   they will know how many bytes you copied into the buffer. If
	   the amount copied was less than the size of the buffer they
	   gave you, this function will be called again with your
	   updated ppos value so you can keep track of where you left off. */
	*ppos += count;

	/* Return the number of characters (bytes) we used from the buffer */
	return count;
}

/* Define which file operations are supported */
/* The full list is in include/linux/fs.h */
static const struct file_operations drvsmpl_procfs1_fops = {
	.owner	= THIS_MODULE,
	.read	= drvsmpl_procfs1_read,
	.write	= drvsmpl_procfs1_write,
};


static int __init drvsmpl_init(void)
{
	int ret = 0;

	DPRINTK("%s called\n",__func__);

	/* Very Simple procfs data */
	/* Create a directory under /proc to hold our virtual files */
	//my_drvsmpl.root_entry = proc_mkdir("drvsmpl", NULL);		/* create dir /proc/drvsmpl */
	my_drvsmpl.root_entry = proc_mkdir("driver/drvsmpl", NULL);	/* create dir /proc/driver/drvsmpl */
	if( my_drvsmpl.root_entry == NULL )
		pr_err("Failed to create proc directory\n");

	/* Create virtual files under the directory we created */
	my_drvsmpl.test1_entry = proc_create("test1", 0, my_drvsmpl.root_entry, &drvsmpl_procfs1_fops);
	if( my_drvsmpl.test1_entry == NULL )
		pr_err("Failed to create proc file\n");

	printk("%s version %s\n", DRIVER_NAME, DRIVER_VERSION);

	return ret;
}

static void __exit drvsmpl_exit(void)
{
	DPRINTK("%s called\n",__func__);

	if ( my_drvsmpl.test1_entry )
		proc_remove( my_drvsmpl.test1_entry);
	if ( my_drvsmpl.root_entry )
		proc_remove( my_drvsmpl.root_entry);

}

module_init(drvsmpl_init);
module_exit(drvsmpl_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chris Brandt");
MODULE_ALIAS("platform:drvsmpl");
MODULE_DESCRIPTION("drvsmpl: Example of a simple Linux driver");

