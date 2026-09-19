#ifndef _REKERNEL_H
#define _REKERNEL_H

#include <linux/sched.h>
#include <linux/sched/jobctl.h>
#include <linux/jump_label.h>

/* netlink constants */
#define NETLINK_REKERNEL_MAX		26
#define NETLINK_REKERNEL_MIN		22
#define USER_PORT			100

/* general constants */
#define PACKET_SIZE			128
#define MIN_USERAPP_UID			(10000)
#define MAX_SYSTEM_UID			(2000)

/* binder async space warn threshold */
#define RESERVE_ORDER			17
#define WARN_AHEAD_SPACE		(1 << RESERVE_ORDER)

/* static key: 启用后所有高频检查都是 0 开销 */
DECLARE_STATIC_KEY_FALSE(rekernel_enabled_key);

static inline bool rekernel_is_ready(void)
{
	return static_branch_unlikely(&rekernel_enabled_key);
}

static inline bool line_is_frozen(struct task_struct *task)
{
	struct task_struct *leader = task->group_leader;

	/*
	 * cgroup v2: 任务已进入 freezer trap
	 * 注意：frozen 是 bit-field，不能用 READ_ONCE（无法取地址）。
	 * 单 bit 读写本身就是原子的，直接读即可。
	 */
	if (leader->frozen)
		return true;

	/* cgroup v2: 冻结请求已排队（jobctl 是 unsigned long，可用 READ_ONCE） */
	if (READ_ONCE(leader->jobctl) & JOBCTL_TRAP_FREEZE)
		return true;

	return false;
}

/* function prototypes */
int start_rekernel_server(void);
int send_netlink_message(char *msg, uint16_t len);

#endif /* _REKERNEL_H */