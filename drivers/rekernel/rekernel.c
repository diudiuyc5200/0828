#include <linux/init.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/proc_fs.h>
#include <linux/freezer.h>
#include <linux/cred.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/jump_label.h>

#include <linux/rekernel.h>

/* static key 定义：配合 rekernel.h 里的 DECLARE_STATIC_KEY_FALSE */
DEFINE_STATIC_KEY_FALSE(rekernel_enabled_key);

static struct sock *rekernel_netlink = NULL;
static DEFINE_MUTEX(rekernel_init_lock);
extern struct net init_net;
static int netlink_unit = NETLINK_REKERNEL_MIN;

int send_netlink_message(char *msg, uint16_t len)
{
	struct sk_buff *skbuffer;
	struct nlmsghdr *nlhdr;

	/* 提前判断 static key，避免无谓的工作 */
	if (!rekernel_is_ready())
		return 0;

	skbuffer = nlmsg_new(len, GFP_ATOMIC);
	if (!skbuffer) {
		printk("netlink alloc failure.\n");
		return -1;
	}
	nlhdr = nlmsg_put(skbuffer, 0, 0, netlink_unit, len, 0);
	if (!nlhdr) {
		printk("nlmsg_put failaure.\n");
		nlmsg_free(skbuffer);
		return -1;
	}
	memcpy(nlmsg_data(nlhdr), msg, len);
	return netlink_unicast(rekernel_netlink, skbuffer, USER_PORT, MSG_DONTWAIT);
}

static void netlink_rcv_msg(struct sk_buff *skbuffer)
{
	/* Ignore recv msg. */
}

static struct netlink_kernel_cfg rekernel_cfg = {
	.input = netlink_rcv_msg,
};

static int rekernel_unit_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", netlink_unit);
	return 0;
}

static int rekernel_unit_open(struct inode *inode, struct file *file)
{
	return single_open(file, rekernel_unit_show, NULL);
}

static const struct file_operations rekernel_unit_fops = {
	.open   = rekernel_unit_open,
	.read   = seq_read,
	.llseek   = seq_lseek,
	.release   = single_release,
	.owner   = THIS_MODULE,
};

static struct proc_dir_entry *rekernel_dir, *rekernel_unit_entry;

int start_rekernel_server(void)
{
	int ret = 0;

	/* Fast path: static key 已启用，直接返回 */
	if (likely(rekernel_is_ready()))
		return 0;

	mutex_lock(&rekernel_init_lock);

	/* Double-check: 可能在等锁期间被其他线程初始化 */
	if (rekernel_netlink != NULL) {
		mutex_unlock(&rekernel_init_lock);
		return 0;
	}

	for (netlink_unit = NETLINK_REKERNEL_MIN;
	     netlink_unit < NETLINK_REKERNEL_MAX; netlink_unit++) {
		rekernel_netlink = (struct sock *)netlink_kernel_create(
			&init_net, netlink_unit, &rekernel_cfg);
		if (rekernel_netlink != NULL)
			break;
	}
	if (rekernel_netlink == NULL) {
		printk("Failed to create Re:Kernel server!\n");
		ret = -1;
		goto out;
	}
	printk("Created Re:Kernel server! NETLINK UNIT: %d\n", netlink_unit);

	rekernel_dir = proc_mkdir("rekernel", NULL);
	if (!rekernel_dir) {
		printk("create /proc/rekernel failed!\n");
	} else {
		char buff[32];
		scnprintf(buff, sizeof(buff), "%d", netlink_unit);
		rekernel_unit_entry = proc_create(buff, 0644, rekernel_dir,
						  &rekernel_unit_fops);
		if (!rekernel_unit_entry)
			printk("create rekernel unit failed!\n");
	}

	/* 启用 static key，让所有高频检查点变为 NOP */
	static_branch_enable(&rekernel_enabled_key);

out:
	mutex_unlock(&rekernel_init_lock);
	return ret;
}