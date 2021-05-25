#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>        //含有iomap函数iounmap函数
#include <asm/uaccess.h>//含有copy_from_user函数
#include <linux/device.h>//含有类相关的处理函数
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

volatile unsigned long *gpiocon = NULL;
volatile unsigned long *gpiodat = NULL;

//static struct class *led_class;
//static struct class_device	*led_class_dev;

static int major;
static struct class *led_class;
static int led_pin;

static unsigned int led_gpio[] = {
    NULL,
    0x01000000+0x1000*25, //PIN1,GPIO1,GPIO_25
    0x01000000+0x1000*10, //PIN2,GPIO2,GPIO_10
    0x01000000+0x1000*42, //PIN3,GPIO3,GPIO_42
    0x01000000+0x1000*11, //PIN4,GPIO4,GPIO_11
    0x01000000+0x1000*24, //PIN5,GPIO5,GPIO_24
};

static int led_open(struct inode *inode, struct file *file)
{
    int base = led_gpio[led_pin];
	gpiocon = (volatile unsigned long *)ioremap(base, 16);
    if (gpiocon) {
		printk("ioremap(0x%x) = 0x%x\n", base, gpiocon);
	}
	else {
		return -EINVAL;
	}
	
    gpiodat = gpiocon + 1;
    
    *gpiocon &= ~(0x3FF);
    *gpiocon |= (0 << 0);
    *gpiocon |= (0 << 2);
    *gpiocon |= (3 << 6);
    *gpiocon |= (1 << 9);

	printk("led_open\n");
	return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	int val;

	//printk("led_write\n");

	copy_from_user(&val, buf, count); //	copy_to_user();

	if (val == 1)
	{
		// 点灯
        *gpiodat &= ~(0x3<< 0);//bit0-1 清零
        *gpiodat |= (0x2 << 0);//点灯
        printk("gpio_25 led1 on \n");

	}
	else
	{
		// 灭灯
        *gpiodat &= ~(0x3<< 0);//bit0-1 清零
        printk("gpio_25 led1 off\n");
	}

	return 0;
}

static int led_release (struct inode *node, struct file *filp)
{
	printk("iounmap(0x%x)\n", gpiocon);
	iounmap(gpiocon);
	return 0;
}

static struct file_operations led_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   led_open,
	.write	=	led_write,
    .release =  led_release,
};

static int led_probe(struct platform_device *pdev)
{
	struct resource		*res;

	/* 根据platform_device的资源进行ioremap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//led_pin = res->start;
    if (res) {
		led_pin = res->start;
	}
	else {
		/* 获得pin属性 */
		of_property_read_u32(pdev->dev.of_node, "pin", &led_pin);
        printk("led_pin = %d",led_pin);
	}

    if (!led_pin) 
	{
		printk("can not get pin for led\n");
		return -EINVAL;
	}

	major = register_chrdev(0, "myled", &led_fops);

	led_class = class_create(THIS_MODULE, "myled");
	device_create(led_class, NULL, MKDEV(major, 0), NULL, "led"); /* /dev/led */
	
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	unregister_chrdev(major, "myled");
	device_destroy(led_class,  MKDEV(major, 0));
	class_destroy(led_class);
	
	return 0;
}

static const struct of_device_id of_match_leds[] = {
	{   
        .compatible = "mdm9607_led", 
        .data = NULL 
    },
};

struct platform_driver led_drv = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver		= {
		.name	= "myled",
        .of_match_table = of_match_leds, ///* 能支持哪些来自于dts的platform_device */ 
	}
};

static int led_init(void)
{
	//major = register_chrdev(0, "myled", &led_fops); // 注册, 告诉内核

	//led_class = class_create(THIS_MODULE, "myled");

	//led_class_dev = class_device_create(led_class, NULL, MKDEV(major, 0), NULL, "led1"); /* /dev/led1 */
	//led_class_dev = device_create(led_class, NULL, MKDEV(major, 0), NULL, "led1"); /* /dev/led1 */

	//gpiocon = (volatile unsigned long *)ioremap((0x01000000+0x1000*25), 16);
	//gpiodat = gpiocon + 1;
    
    platform_driver_register(&led_drv);
	return 0;
}

static void led_exit(void)
{
	//unregister_chrdev(major, "myled"); // 卸载

	//class_device_unregister(led_class_dev);
	//device_unregister(led_class_dev);
	//class_destroy(led_class);
	//iounmap(gpiocon);
    platform_driver_unregister(&led_drv);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");//不加的话加载会有错误提醒
MODULE_AUTHOR("puck.shen@quectel.com");//作者
MODULE_VERSION("v03");//版本
MODULE_DESCRIPTION("This is led_drv for dts!");//简单的描述

