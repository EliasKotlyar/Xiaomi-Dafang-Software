/*-----------------------------------------------------------------------------
	 This confidential and proprietary software/information may be used only
		as authorized by a licensing agreement from Apical Limited

				   (C) COPYRIGHT 2011 - 2015 Apical Limited
						  ALL RIGHTS RESERVED

	  The entire notice above must be reproduced on all authorized
	   copies and copies may only be made to the extent permitted
			 by a licensing agreement from Apical Limited.
-----------------------------------------------------------------------------*/
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/kfifo.h>
#include "system_chardev.h"
#include "log.h"

#define SYSTEM_CHARDEV_FIFO_SIZE 4096
#define SYSTEM_CHARDEV_NAME "isp"

/* fifo from user to fw */
static DECLARE_KFIFO(isp_kfifo_in, char, SYSTEM_CHARDEV_FIFO_SIZE);

/* fifo from fw to user */
static DECLARE_KFIFO(isp_kfifo_out, char, SYSTEM_CHARDEV_FIFO_SIZE);

/* sync between fw and user */
static DEFINE_MUTEX(isp_lock);

struct isp_client_state {
	int id;
	int opened;
};

#define ISP_CLIENT_INITIAL_STATE {0, 0}

/* the actual state of the character device, updated immediately*/
static struct isp_client_state isp_client_state_kernel = ISP_CLIENT_INITIAL_STATE;

/* last accessed state of the character device, updated only on fw call */
static struct isp_client_state isp_client_state_fw = ISP_CLIENT_INITIAL_STATE;

/* only one client available at a time */
static int isp_client_state_open(struct isp_client_state *state)
{
	if (state->opened)
		return -EBUSY;

	state->id = state->id + 1;
	state->opened = 1;

	return 0;
}

static void isp_client_state_release(struct isp_client_state *state)
{
	state->opened = 0;
}

static int isp_client_state_equal(struct isp_client_state *left, struct isp_client_state *right)
{
	return (left->id == right->id && left->opened == right->opened);
}

static void isp_client_state_assign(struct isp_client_state *left, struct isp_client_state *right)
{
	left->id = right->id;
	left->opened = right->opened;
}

static int isp_fops_open(struct inode *inode, struct file *f)
{
	int rc;

	rc = mutex_lock_interruptible(&isp_lock);
	if (rc)
		goto lock_failure;

	rc = isp_client_state_open(&isp_client_state_kernel);
	if (rc)
		goto open_failure;

	kfifo_reset(&isp_kfifo_in);
	kfifo_reset(&isp_kfifo_out);

open_failure:
	mutex_unlock(&isp_lock);

lock_failure:
	return rc;
}

static int isp_fops_release(struct inode *inode, struct file *f)
{
	int rc;

	rc = mutex_lock_interruptible(&isp_lock);
	if (rc)
		return rc;

	isp_client_state_release(&isp_client_state_kernel);

	mutex_unlock(&isp_lock);

	return 0;
}

static ssize_t isp_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	int rc;
	unsigned int copied;

	if (mutex_lock_interruptible(&isp_lock))
		return -ERESTARTSYS;

	rc = kfifo_from_user(&isp_kfifo_in, buf, count, &copied);

	mutex_unlock(&isp_lock);

	return rc ? rc : copied;
}

static ssize_t isp_fops_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int rc;
	unsigned int copied;

	if (mutex_lock_interruptible(&isp_lock))
		return -ERESTARTSYS;

	rc = kfifo_to_user(&isp_kfifo_out, buf, count, &copied);

	mutex_unlock(&isp_lock);

	return rc ? rc : copied;
}

static struct file_operations isp_fops = {
	.owner = THIS_MODULE,
	.open = isp_fops_open,
	.release = isp_fops_release,
	.read = isp_fops_read,
	.write = isp_fops_write,
	.llseek = noop_llseek
};


static struct miscdevice isp_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SYSTEM_CHARDEV_NAME,
	.fops = &isp_fops
};

int system_chardev_init(void)
{
	int rc;

	INIT_KFIFO(isp_kfifo_in);
	INIT_KFIFO(isp_kfifo_out);

	rc = misc_register(&isp_miscdevice);
	if (rc)
		return -1;

	return 0;
}

int system_chardev_read(char *data, int size)
{
	int rc;

	mutex_lock(&isp_lock);

	if (!isp_client_state_equal(&isp_client_state_fw, &isp_client_state_kernel)) {
		isp_client_state_assign(&isp_client_state_fw, &isp_client_state_kernel);
		rc = -1;
		goto done;
	}

	rc = kfifo_out(&isp_kfifo_in, data, size);

done:
	mutex_unlock(&isp_lock);
	return rc;
}

int system_chardev_write(const char *data, int size)
{
	int rc;

	mutex_lock(&isp_lock);

	if (!isp_client_state_equal(&isp_client_state_fw, &isp_client_state_kernel)) {
		isp_client_state_assign(&isp_client_state_fw, &isp_client_state_kernel);
		rc = -1;
		goto done;
	}

	rc = kfifo_in(&isp_kfifo_out, data, size);

done:
	mutex_unlock(&isp_lock);
	return rc;
}

int system_chardev_destroy(void)
{
	int rc = misc_deregister(&isp_miscdevice);
	return rc;
}
