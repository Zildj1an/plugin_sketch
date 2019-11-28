#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/percpu.h>       /* CPU-local allocations */
#include <linux/sched.h>        /* struct task_struct    */
#include <linux/list.h>
#include <linux/slab.h>		/* kmalloc */
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>	/* copy_(from/to)_user */

/* #include <some_libraries_TODO_API> */
#include <litmus/preempt.h>
#include <litmus/sched_plugin.h>

#include <litmus/litmus.h>      /* API definitions       */
#include <litmus/jobs.h>
#include <litmus/budget.h>

/* Functions for task state changes*/
#include <litmus/jobs.h>
#include <litmus/budget.h>

/* Special ADT from Juan Carlos Saez */
#include "list_jcsaez.h"

#define MAX_SIZE 	    500
#define MODULE_NAME         "MI_PLUGIN"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Carlos Bilbao");
MODULE_DESCRIPTION("The FCFS scheduling algorithm");

/* My /proc file entry */
static struct proc_dir_entry *my_proc_entry;

/* FCFS-queue node */
struct fcfs_queue_node {
        struct task_struct* task;
        struct list_head links;
};

/* CPU state */
struct cpu_state {
        int cpu;
        struct task_struct* scheduled;
        spinlock_t queue_lock;
        /* FCFS-queue special queue*/
        sized_list_t fcfs_queue;
        struct list_head link_queue;
};

static DEFINE_PER_CPU(struct cpu_state, cpu_state);
#define cpu_state_for(cpu_id)   (&per_cpu(cpu_state, cpu_id))
#define local_cpu_state()       (this_cpu_ptr(&cpu_state))

static inline void add_tail(struct cpu_state* local_state, struct task_struct* task) {

	struct fcfs_queue_node *node;
	node = kmalloc(sizeof(struct fcfs_queue_node), GFP_KERNEL);
        node->task = task;
        insert_sized_list_tail(&local_state->fcfs_queue,node);
}

static void remove_List(void) {

	int cpu;
        unsigned long flags;
        struct cpu_state *state;

        for_each_online_cpu(cpu){
                printk(KERN_INFO "Removing list from CPU %d...\n",cpu);
                state = cpu_state_for(cpu);
                spin_lock_irqsave(&state->queue_lock, flags);
		remove_sized_list_all(&state->fcfs_queue);
                spin_unlock_irqrestore(&state->queue_lock, flags);
        }
}

static long nuevo_activate_plugin(void) {

        int cpu;
	unsigned long flags;
        struct cpu_state *state;

        for_each_online_cpu(cpu){
                printk(KERN_INFO "Initializing CPU %d...\n",cpu);
                state = cpu_state_for(cpu);
                state->cpu = cpu;
	        state->scheduled = NULL;
		spin_lock_irqsave(&state->queue_lock, flags);
                init_sized_list(&state->fcfs_queue,offsetof(struct fcfs_queue_node,
	                          links));
        	spin_unlock_irqrestore(&state->queue_lock, flags);
	}
        try_module_get(THIS_MODULE); /* Increase counter */

    return 0;
}

static long nuevo_deactivate_plugin(void) {

    module_put(THIS_MODULE); /* Decrease counter */
    remove_List();

    return 0;
}

static struct task_struct* nuevo_schedule(struct task_struct *prev) {

	struct cpu_state *local_state = local_cpu_state();
	unsigned long flags;
        int exists, self_suspends = 0, preempt, resched; /* Prev's task state */
        struct task_struct *next = NULL; /* NULL schedules background work */
	struct fcfs_queue_node *next_node;

	spin_lock_irqsave(&local_state->queue_lock, flags);

        BUG_ON(local_state->scheduled && local_state->scheduled != prev);

        exists = local_state->scheduled != NULL;
//      self_suspends = exists && !is_current_running(); Ignore this function (jobs.c)

        /* preempt is true if task `prev` has lower priority than something on
         * the FSCFS ready queue. In this case we will assume this true.*/
        preempt = 1;
        /* check all conditions that make us reschedule */
        resched = preempt;
        /* if `prev` suspends, it CANNOT be scheduled anymore => reschedule */
        if (self_suspends) resched = 1;

        if (resched) {
                /* The previous task goes to the ready queue back if it did not self_suspend) */
                if (likely(exists && !self_suspends)) {
			add_tail(local_state, local_state->scheduled);
		}

		if (likely(sized_list_length(&local_state->fcfs_queue))) {
			next_node = head_sized_list(&local_state->fcfs_queue);
			next = next_node->task;
			remove_sized_list(&local_state->fcfs_queue,&next_node);
		}
	} else {
		next = local_state->scheduled;	 /* No preemption is required. */
        }

        local_state->scheduled = next;

        if (exists && prev != next) {
		printk(KERN_INFO "CPU [%d] -> Previous task descheduled.\n", local_state->cpu);
	}
        if (next) {
		printk(KERN_INFO "CPU [%d] -> New task scheduled.\n", local_state->cpu);
	}

        /* This mandatory. It triggers a transition in the LITMUS^RT remote
         * preemption state machine. Call this AFTER the plugin has made a local
         * scheduling decision.
         */
        sched_state_task_picked();

	spin_unlock_irqrestore(&local_state->queue_lock, flags);

        return next;
}

/* ============================================
   =========== Possible task states ===========
   ============================================
*/

static void nuevo_task_new(struct task_struct *tsk, int on_runqueue,int is_running) {

        unsigned long flags; /* IRQ flags. */
        struct cpu_state *local_state = cpu_state_for(get_partition(tsk));

        printk(KERN_INFO "CPU [%d] -> There is a new task (on runqueue:%d, running:%d, time: %llu)\n",
                               local_state->cpu,on_runqueue, is_running, ktime_to_ns(ktime_get()));
        spin_lock_irqsave(&local_state->queue_lock, flags);

        if (is_running) {
                BUG_ON(local_state->scheduled != NULL);
                local_state->scheduled = tsk;
        } else if (on_runqueue) {
		add_tail(local_state, tsk);
	}
        spin_unlock_irqrestore(&local_state->queue_lock, flags);
}

static void nuevo_task_exit(struct task_struct *tsk) {

        unsigned long flags;
        struct cpu_state *local_state = cpu_state_for(get_partition(tsk));

        spin_lock_irqsave(&local_state->queue_lock, flags);

        /* For simplicity, we assume here that the task is no longer queued anywhere else. This
         * is the case when tasks exit by themselves; additional queue management is
         * is required if tasks are forced out by other tasks. */

        if (local_state->scheduled == tsk)
                local_state->scheduled = NULL;

        spin_unlock_irqrestore(&local_state->queue_lock, flags);
}

static void nuevo_task_resume(struct task_struct  *tsk) {

        unsigned long flags;
        struct cpu_state *local_state = cpu_state_for(get_partition(tsk));

        printk("CPU [%d] Task woke up at %llu\n", local_state->cpu, ktime_to_ns(ktime_get()));

        spin_lock_irqsave(&local_state->queue_lock, flags);

        /* Check required to avoid races with tasks that resume before
         * the scheduler "noticed" that it resumed. That is, the wake up may
         * race with the call to schedule(). */
        if (local_state->scheduled != tsk) {
		add_tail(local_state, tsk);
        }

        spin_unlock_irqrestore(&local_state->queue_lock, flags);
}

static long nuevo_admit_task(struct task_struct *tsk) {

	int ret = -EINVAL;

        if (task_cpu(tsk) == get_partition(tsk)) {
                printk(KERN_INFO "CPU [%d] ->  New task admitted \n",
                        cpu_state_for(get_partition(tsk))->cpu);
                ret = 0;
        }
        return ret;
}

static struct sched_plugin nuevo_plugin = {
	.plugin_name 	    = "NUEVO",
        .schedule           = nuevo_schedule,
        .admit_task         = nuevo_admit_task,
	.activate_plugin    = nuevo_activate_plugin,
        .deactivate_plugin  = nuevo_deactivate_plugin,
	.task_new           = nuevo_task_new,
        .task_exit          = nuevo_task_exit,
        .task_wake_up       = nuevo_task_resume,
};

/* PROC FS */
static ssize_t nuevo_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {

    int cpu, read = 0;
    struct cpu_state *local_state; char sp[] = "     ";
    char kbuf[MAX_SIZE]; char cpu_display[] = "CPU     NUM_SCHEDULES";

    if ((*off) > 0) return 0; //Previously invoked!

    read += sprintf(&kbuf[read],"%s", cpu_display);

    for_each_online_cpu(cpu){
        local_state = cpu_state_for(cpu);
	read += sprintf(&kbuf[read],"%i%s%i\n",cpu,sp,(int)sized_list_length(&local_state->fcfs_queue));
        kbuf[read++] = '\n';
    }

    kbuf[read++] = '\0';

    if (copy_to_user(buf, kbuf, read) > 0) return -EFAULT;
    (*off) += read;

 return read;
}

/* Operations that the module can handle */
const struct file_operations fops = {.read = nuevo_read,};

static int __init init_nuevo(void){

	int ret;

        if ((my_proc_entry = proc_create("fcfs_althm", 0666, NULL, &fops)) == NULL){
		ret = -ENOMEM;
		printk(KERN_INFO "Couldn't create the /proc entry \n");
	}
	else {
        	printk(KERN_INFO "[0] Module %s loaded \n", MODULE_NAME);
	 	ret = register_sched_plugin(&nuevo_plugin);
	}

  return ret;
}

static void exit_nuevo(void) {

    /* If the counter is zero, this will be called
    *  We might need to remove /proc info in the future here
    */
    unregister_sched_plugin(&nuevo_plugin);
    remove_proc_entry("fcfs_althm", NULL);
    printk(KERN_INFO "Plugin %s removed\n",MODULE_NAME);
}

module_init(init_nuevo);
module_exit(exit_nuevo);

