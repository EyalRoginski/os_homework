#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/fs.h> /* for register_chrdev */
#include <linux/init.h>
#include <linux/kernel.h>  /* We're doing kernel work */
#include <linux/module.h>  /* Specifically, a module */
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/uaccess.h> /* for get_user and put_user */

#define MAX_MESSAGE_SLOTS 256
#define CHANNEL_BUF_LENGTH 128
#define MAJOR_NUM 235
#define DEVICE_NAME "message_slot"

MODULE_LICENSE("GPL");

struct channel_t {
    unsigned int id;
    char buf[CHANNEL_BUF_LENGTH];
    struct channel_t *next;
};

struct message_slot_t {
    struct channel_t *channels;
};

struct message_slot_t message_slots[MAX_MESSAGE_SLOTS];

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "open message_slot");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t length, loff_t *offset) {
    printk(KERN_INFO "read message_slot");
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
    printk(KERN_INFO "write message_slot");
    return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id,
                         unsigned long ioctl_param) {
    printk(KERN_INFO "ioctl message_slot");
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
};

static int __init init(void) {
    int i;
    int register_return = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (register_return < 0) {
        printk(KERN_ERR "%s registration failed for %d", DEVICE_NAME,
               MAJOR_NUM);
        return register_return;
    }

    for (i = 0; i < MAX_MESSAGE_SLOTS; i++) {
        message_slots[i].channels = NULL;
    }
    return 0;
}
static void __exit cleanup(void) {}

module_init(init);
module_exit(cleanup);
