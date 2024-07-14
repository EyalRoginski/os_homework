#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/fs.h> /* for register_chrdev */
#include <linux/init.h>
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/slab.h>
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/uaccess.h> /* for get_user and put_user */

#include "message_slot.h"

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

struct channel_t *new_channel(unsigned long id) {
    struct channel_t *new_channel =
        kmalloc(sizeof(struct channel_t), GFP_KERNEL);
    if (!new_channel)
        return new_channel;
    new_channel->id = id;
    memset(new_channel->buf, 0, CHANNEL_BUF_LENGTH);
    new_channel->next = NULL;
    return new_channel;
}

/**
 * Searches the linked list of channels of the slot with the given minor number,
 * for a channel with the given id. Returns a pointer to that channel, creating
 * it if it doesn't exist, and `create != 0`. Returns NULL if there was an
 * error, or if `create = 0` and the channel wasn't found.
 * */
struct channel_t *find_channel(unsigned long id, unsigned int minor_num,
                               int create) {
    struct channel_t *channel = message_slots[minor_num].channels;
    while (channel && channel->id != id && channel->next) {
        channel = channel->next;
    }
    if (!channel) {
        if (!create) {
            return NULL;
        }
        // No channels written to yet
        message_slots[minor_num].channels = new_channel(id);
        channel = message_slots[minor_num].channels;
    } else if (channel->id != id) {
        if (!create) {
            return NULL;
        }
        // No writes to this channel yet
        // At end of list
        channel->next = new_channel(id);
        channel = channel->next;
    }
    return channel;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
    struct channel_t *channel;
    unsigned int minor_num = iminor(file->f_inode);
    unsigned long id = (unsigned long)file->private_data;
    if (id == 0) {
        return -EINVAL;
    }
    if (length == 0 || length > CHANNEL_BUF_LENGTH) {
        return -EMSGSIZE;
    }
    channel = find_channel(id, minor_num, 1);
    if (!channel) {
        // Probably problem with memory allocation
        return -ENOMEM;
    }
    memcpy(channel->buf, buffer, length);
    return length;
}

static long device_ioctl(struct file *file, unsigned int ioctl_command_id,
                         unsigned long ioctl_param) {
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0) {
        return -EINVAL;
    }

    file->private_data = (void *)ioctl_param;
    return 0;
}

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "open message_slot");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t length, loff_t *offset) {
    printk(KERN_INFO "read message_slot");
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
        printk(KERN_ERR "%s registration failed for %d, with return value %d",
               DEVICE_NAME, MAJOR_NUM, register_return);
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
