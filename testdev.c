/*
 
COP 4600 - Operating Systems
Programming Assignment 2

A simple kernel module: a charater-mode device driver
 */
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>		//needed for character driver init
#include <linux/init.h>
#include <linux/uaccess.h>	//needed for copy_to_user

#define DEVICE_NAME "testdev"
#define BUFF_SIZE 1024

//License and author info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Group 18");
MODULE_DESCRIPTION("A simple character-mode Driver.");

static int majorNumber;
static char message[BUFF_SIZE] = {0};
static int messageSize;

//prototypes for file ops
static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode* inodep, struct file* filep);
static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset);
static ssize_t dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset);

static struct file_operations fops = 
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release
};


//Initialization: register a Major number, save it for later mknod
static int __init testdev_init(void) {
	//Request Major device number
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	
	if (majorNumber < 0) {
		printk(KERN_ALERT "testdev: Failed to register a major version number.\n");
		return majorNumber;
	}
	printk(KERN_INFO "testdev: Registered major version number %d.\n", majorNumber);
	strcpy(message, "");
	return 0;
}
module_init(testdev_init);


//Exit: unregister the Major number
static void __exit testdev_exit(void) {
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "testdev: Unregistered major version number %d.\n", majorNumber);
}
module_exit(testdev_exit);


//Open: open the device
static int dev_open(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev: Device opened.\n");
	return 0;
}

//Release: close the device
static int dev_release(struct inode* inodep, struct file* filep) {
	printk(KERN_INFO "testdev: Device closed.\n");
	return 0;
}

//Write: write information to the device
static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset) {
	int space = BUFF_SIZE - strlen(message);
	if(space > 0) {
		strncpy(message, buffer, space);   // appending received string
		messageSize = strlen(message);    // store the length of the stored message
		if(space >= len) {
			printk(KERN_INFO "testdev: Received %zu characters from the user\n", len);
			return len;
		} else {
			printk(KERN_INFO "testdev: Received %zu characters from the user. Buffer full\n", space);
			return space;
		}
	} else {
		printk(KERN_INFO "testdev: Cannot receive characters from the user. Buffer full.\n");
		return -1;
	}
}

//Read: read information from the device
static ssize_t dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset) {
	//TODO: Stub
	int err =0;
	if(len > strlen(message)) {
		err = copy_to_user(buffer, message, strlen(message));
		if(err == 0) {
			printk(KERN_INFO "testdev: Insufficient characters. Sent %d characters to user.\n", strlen(message));
			strcpy(message, "");
			return 0;
		} else {
			printk(KERN_INFO "testdev: Error, failed to send characters to user.\n");
			return err;
		}
	} else {
		err = copy_to_user(buffer, message, len);
		if(err == 0) {
			printk(KERN_INFO "testdev: Sent %d characters to user.\n", len);
			//I dislike messing with strings as char arrays directly, but it seems that's the only choice for substrings in c.
			//TODO: Edge cases need to be tested thoroughly, this is very prone to off-by-one errors or accesses beyond the terminating null.
			memcpy(message, &message[len], strlen(message) - len);
			return 0;
		} else {
			printk(KERN_INFO "testdev: Error, failed to send characters to user.\n");
			return err;
		}
	}
}