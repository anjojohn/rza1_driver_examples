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

#define COLOR_RED 0
#define COLOR_GRN 1
#define COLOR_BLU 2


#define VDC5_0_BASE 0xFCFF7500
#define VDC5_1_BASE 0xFCFF9500
#define VDC5_BASE_RANGE 0x1000

/* Layer offsets from BASE */
const u32 layer_offset[4] = {
	0x100,	/* GR0_OFFSET */
	0x800,	/* GR1_OFFSET */
	0x200,	/* GR2_OFFSET */
	0x280	/* GR3_OFFSET */
};

/* VDC5 Layer Registers (offset from layer_offset[] ) */
#define GRx_UPDATE	0x0000
#define GRx_FLM_RD	0x0004
#define GRx_FLM1	0x0008
#define GRx_FLM2	0x000C
#define GRx_FLM3	0x0010
#define GRx_FLM4	0x0014
#define GRx_FLM5	0x0018
#define GRx_FLM6	0x001C
#define GRx_AB1		0x0020
#define GRx_AB2		0x0024
#define GRx_AB3		0x0028
#define GRx_AB4		0x002C
#define GRx_AB5		0x0030
#define GRx_AB6		0x0034
#define GRx_AB7		0x0038
#define GRx_AB8		0x003C
#define GRx_AB9		0x0040
#define GRx_AB10	0x0044
#define GRx_AB11	0x0048
#define GRx_BASE	0x004C
#define GRx_CLUT	0x0050
#define GRx_MON		0x0054

/* VDC5 Output Controller Registers (offset from VDC5_x_BASE) */
#define OUT_UPDATE	0x500
#define OUT_BRIGHT1	0x508
#define OUT_BRIGHT2	0x50C
#define OUT_CONTRAST	0x510


/* Our Private Data Structure */
struct drvsmpl {
	int	layer;
	void * __iomem *reg;
};

/* Global variables for this driver only */
static struct drvsmpl my_drvsmpl;
static struct miscdevice my_dev;


/* Register Read/Write functions */
static inline u32 drvsmpl_read32(struct drvsmpl *pd, u16 offset)
{
	return ioread32((u8*)pd->reg + offset);
}
static inline void drvsmpl_write32(struct drvsmpl *pd, u32 val, u16 offset)
{
	iowrite32(val, (u8*)pd->reg + offset);
}
static inline u32 drvsmpl_read16(struct drvsmpl *pd, u16 offset)
{
	return ioread16((u8*)pd->reg + offset);
}
static inline void drvsmpl_write16(struct drvsmpl *pd, u16 val, u16 offset)
{
	iowrite16(val, (u8*)pd->reg + offset);
}
static inline u32 drvsmpl_read8(struct drvsmpl *pd, u16 offset)
{
	return ioread8((u8*)pd->reg + offset);
}
static inline void drvsmpl_write8(struct drvsmpl *pd, u8 val, u16 offset)
{
	iowrite8(val, (u8*)pd->reg + offset);
}

/* Define which file operations are supported */
/* The full list is in include/linux/fs.h */
struct file_operations drvsmpl_fops = {
	.owner		=	THIS_MODULE,
	.read		=	NULL,
	.write		=	NULL,
	.unlocked_ioctl	=	NULL,
	.open		=	NULL,
	.release	=	NULL,
};


static int drvsmpl_get_contrast( struct drvsmpl *pd, int rgb)
{
	int val;

	val = drvsmpl_read32( &my_drvsmpl, OUT_CONTRAST);

	switch(rgb) {
	case COLOR_RED: /* red */
		val = (val >> 0) & 0xFF;
		break;
	case COLOR_BLU: /* blue */
		val = (val >> 8) & 0xFF;
		break;
	case COLOR_GRN: /* green */
		val = (val >> 16) & 0xFF;
		break;
	}
	
	val = val - 128; // convert to 8-bit pos/neg value

	return val;
}
static void drvsmpl_set_contrast( struct drvsmpl *pd, int rgb, int val)
{
	u32 reg_val;

	/* Valid input values are +127 to -128 */

	/* Restrict input value to 8 bits */
	if( val > 127 )
		val = 127;
	if( val < -128 )
		val = -128;

	/* convert to 8-bit signed number */
	val = (val + 128) & 0xFF;

	reg_val = drvsmpl_read32( &my_drvsmpl, OUT_CONTRAST);
	switch(rgb) {
	case COLOR_RED: /* red */
		reg_val &= 0xFFFFFF00;	// clear out red
		reg_val |= (val << 0);
		break;
	case COLOR_BLU: /* blue */
		reg_val &= 0xFFFF00FF;	// clear out blue
		reg_val |= (val << 8);
		break;
	case COLOR_GRN: /* green */
		reg_val &= 0xFF00FFFF;	// clear out green
		reg_val |= (val << 16);
		break;
	}

	drvsmpl_write32( &my_drvsmpl, reg_val, OUT_CONTRAST);
	drvsmpl_write32( &my_drvsmpl, 0x1, OUT_UPDATE);

}

static ssize_t drvsmpl_show_layer(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/* Return the number characters (bytes) copied to the buffer */
	return sprintf(buf, "%d\n", my_drvsmpl.layer);
}


static ssize_t drvsmpl_store_layer(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	/* Scan in our argument(s) */
	sscanf(buf, "%d", &my_drvsmpl.layer);

	/* Return the number of characters (bytes) we used from the buffer */
	return count;
}


static ssize_t drvsmpl_store_fade(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int fade;
	u32 val;
	int lay_off = layer_offset[ my_drvsmpl.layer ];

	/* Scan in our argument(s) */
	sscanf(buf, "%d", &fade);

	/* Valid input values are -255 to 255 */
	/* A fade coefficient value of 0 does not do anything (x + 0 = 0) */
	/* Restrict input value to 8 bits */
	if( fade > 255 )
		val = 255;
	if( fade < -255 )
		val = -255;

	/* Define the alpha rectangle to be the same dimensions and location
	   as the main frame buffer */

	/* GR_AB4 = GR_AB2 */
	val = drvsmpl_read32( &my_drvsmpl, lay_off + GRx_AB2);
	drvsmpl_write32( &my_drvsmpl, val ,lay_off + GRx_AB4);

	/* GR_AB5 = GR_AB3 */
	val = drvsmpl_read32( &my_drvsmpl, lay_off + GRx_AB3);
	drvsmpl_write32( &my_drvsmpl, val ,lay_off + GRx_AB5);

	/* Enable alpha blending in a rectangular area and blending with lower layer */
	drvsmpl_write32( &my_drvsmpl, 0x00001003 ,lay_off + GRx_AB1);

	if (fade <= 0) 
	{
		/* Fade out to the next lower layer */
		fade = 0 - fade;	// Make positive number
		drvsmpl_write32( &my_drvsmpl, 0x01000001 | (fade << 16 ), lay_off + GRx_AB6);
	}
	else {
		/* Fade back in from the next lower layer */
		drvsmpl_write32( &my_drvsmpl, 0x00000001 | (fade << 16), lay_off + GRx_AB6);
	}

	/* latch the new value */
	drvsmpl_write32( &my_drvsmpl, 0x111, lay_off + GRx_UPDATE);

	/* Return the number of characters (bytes) we used from the buffer */
	return count;
}


static ssize_t drvsmpl_show_contrast_red(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", drvsmpl_get_contrast(&my_drvsmpl, COLOR_RED) );
}
static ssize_t drvsmpl_store_contrast_red(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val;

	sscanf(buf, "%d", &val);
	drvsmpl_set_contrast( &my_drvsmpl, COLOR_RED, val);
	return count;
}
static ssize_t drvsmpl_show_contrast_blue(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", drvsmpl_get_contrast(&my_drvsmpl, COLOR_BLU) );
}
static ssize_t drvsmpl_store_contrast_blue(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val;

	sscanf(buf, "%d", &val);
	drvsmpl_set_contrast( &my_drvsmpl, COLOR_BLU, val);
	return count;
}
static ssize_t drvsmpl_show_contrast_green(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", drvsmpl_get_contrast(&my_drvsmpl, COLOR_GRN) );
}
static ssize_t drvsmpl_store_contrast_green(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int val;

	sscanf(buf, "%d", &val);
	drvsmpl_set_contrast( &my_drvsmpl, COLOR_GRN, val);
	return count;
}


/* Table for sysfs entires */
static struct device_attribute drvsmpl_device_attributes[] = {
	__ATTR(	layer, 0666, drvsmpl_show_layer, drvsmpl_store_layer),
	__ATTR(	fade, 0222, NULL, drvsmpl_store_fade),
	__ATTR(	contrast_red, 0666, drvsmpl_show_contrast_red, drvsmpl_store_contrast_red),
	__ATTR(	contrast_blue, 0666, drvsmpl_show_contrast_blue, drvsmpl_store_contrast_blue),
	__ATTR(	contrast_green, 0666, drvsmpl_show_contrast_green, drvsmpl_store_contrast_green),
};


static int __init drvsmpl_init(void)
{
	int ret = 0;
	int i;

	DPRINTK("%s called\n",__func__);

	/* Remap registers addresses in physical space (not accessible)
	 * to virtual address that are accessible. */
	my_drvsmpl.reg = ioremap(VDC5_0_BASE, VDC5_BASE_RANGE);
	//my_drvsmpl.reg = ioremap(VDC5_1_BASE, VDC5_BASE_RANGE);

	if (my_drvsmpl.reg == NULL) {
		ret = -ENOMEM;
		printk("ioremap error.\n");
		goto clean_up;
	}

	/* Register our driver */
	my_dev.minor = MISC_DYNAMIC_MINOR;
	my_dev.name = "vdc5_extra";	// will show up as /dev/vdc5_extra
	my_dev.fops = &drvsmpl_fops;
	ret = misc_register(&my_dev);
	if (ret) {
		printk(KERN_ERR "Misc Device Registration Failed\n");
		goto clean_up;
	}

	/* Add our sysfs interface virtual files to the system */
	/* NOTE: The device_create_file() feature can only be used with GPL drivers */
	for (i = 0; i < ARRAY_SIZE(drvsmpl_device_attributes); i++) {
		ret = device_create_file(my_dev.this_device, &drvsmpl_device_attributes[i]);
		if (ret < 0) {
			printk(KERN_ERR "device_create_file error\n");
			break;
		}
	}

	/* Defaults */
	my_drvsmpl.layer = 2;	/* /dev/fb0 in the BSP */

	printk("%s version %s\n", DRIVER_NAME, DRIVER_VERSION);

	return 0;

clean_up:
	if (my_drvsmpl.reg)
		iounmap(my_drvsmpl.reg);

	if( my_dev.this_device ) {
		/* Remove our sysfs files */
		for (i = 0; i < ARRAY_SIZE(drvsmpl_device_attributes); i++)
			device_remove_file(my_dev.this_device, &drvsmpl_device_attributes[i]);

		misc_deregister(&my_dev);
	}

	return ret;
}

static void __exit drvsmpl_exit(void)
{
	int i;

	DPRINTK("%s called\n",__func__);

	if (my_drvsmpl.reg)
		iounmap(my_drvsmpl.reg);

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

