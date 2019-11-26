#include <linux/module.h>
#include <linux/percpu.h>       /* CPU-local allocations */
#include <linux/sched.h>        /* struct task_struct    */
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

/* #include <some_libraries_TODO_API> */
#include <litmus/preempt.h>
#include <litmus/sched_plugin.h>
#include <litmus/litmus.h>      /* API definitions       */
#include <litmus/jobs.h>
#include <litmus/budget.h>

/* Functions for task state changes*/
#include <litmus/jobs.h>
#include <litmus/budget.h>

#define MODULE_NAME         "MI_PLUGIN"

/* FCFS-queue node */
struct fcfs_queue_node {
        struct task_struct* task;
        struct list_head links;
};

/* CPU state */
struct cpu_state {
        int cpu;
	int num_tasks_queued;
        struct task_struct* scheduled;
        spinlock_t queue_lock;
        /* FCFS-queue ghost node*/
        struct list_head ghost_node;
};

static DEFINE_PER_CPU(struct cpu_state, cpu_state);
#define cpu_state_for(cpu_id)   (&per_cpu(cpu_state, cpu_id))
#define local_cpu_state()       (this_cpu_ptr(&cpu_state))

static long nuevo_activate_plugin(void) {

        int cpu;
        struct cpu_state *state;

        for_each_online_cpu(cpu){
                printk(KERN_INFO "Initializing CPU %d...\n",cpu);
                state = cpu_state_for(cpu);
                state->cpu = cpu;
		state->num_tasks_queued = 0;
                state->queue_lock = SPIN_LOCK_UNLOCKED;
	        state->scheduled = NULL;
                INIT_LIST_HEAD(&state->ghost_node);
        }
        try_module_get(THIS_MODULE); /* Increase counter */

    return 0;
}

static long nuevo_deactivate_plugin(void) {
    module_put(THIS_MODULE); /* Decrease counter */
    return 0;
}

static struct task_struct* nuevo_schedule(struct task_struct *prev) {

	struct cpu_state *local_state = local_cpu_state();
	unsigned long flags;
        int exists, self_suspends, preempt, resched; /* Prev's task state */
        struct task_struct *next = NULL; /* NULL schedules background work */
	struct fcfs_queue_node prev_node;

	spin_lock_irqsave(&state->queue_lock, flags);

        BUG_ON(local_state->scheduled && local_state->scheduled != prev);

        exists = local_state->scheduled != NULL;
        self_suspends = exists && !is_current_running();
        /* preempt is true if task `prev` has lower priority than something on
         * the FSCFS ready queue. In this case we will assume this true.*/
        preempt = 1;
        /* check all conditions that make us reschedule */
        resched = preempt;
        /* if `prev` suspends, it CANNOT be scheduled anymore => reschedule */
        if (self_suspends) resched = 1;
        if (resched) {
                /* The previous task goes to the ready queue back if it did not self_suspend) */
                if (exists && !self_suspends) {
			// Add it to the back
			prev_node = vmalloc(sizeof(struct fcfs_queue_node));
			prev_node.task = &local_state->scheduled;
			list_add_tail(&prev_node.links,&local_state->ghost_node);
                	local_state->num_tasks_queued++;
		}

		if (local_state->num_tasks_queued) {
			next = list_entry(&local_state->ghost_node->next, struct fcfs_queue_node, links)->task;
			list_del(list_entry(&local_state->ghost_node->next, struct fcfs_queue_node, links));
        		local_state->num_tasks_queued--;
		}
	} else {
		next = local_state->scheduled;	 /* No preemption is required. */
        }

        local_state->scheduled = next;

        if (exists && prev != next) printk(KERN_INFO "CPU %d -> Previous task descheduled.\n", local_state->cpu);
        if (next) printk(KERN_INFO "CPU %d -> New task scheduled.\n", local_state->cpu);

        /* This mandatory. It triggers a transition in the LITMUS^RT remote
         * preemption state machine. Call this AFTER the plugin has made a local
         * scheduling decision.
         */
        sched_state_task_picked();

	spin_unlock_irqrestore(&state->queue_lock, flags);

        return next;
}

static long nuevo_admit_task(struct task_struct *tsk) {

  /* Reject every task for now */
  return -EINVAL;
}

static struct sched_plugin nuevo_plugin = {
	.plugin_name 	    = "NUEVO",
        .schedule           = nuevo_schedule,
        .admit_task         = nuevo_admit_task,
	.activate_plugin    = nuevo_activate_plugin,
        .deactivate_plugin  = nuevo_deactivate_plugin
};

static int __init init_nuevo(void){

    printk(KERN_INFO "[0] Module %s loaded \n", MODULE_NAME);

  return register_sched_plugin(&nuevo_plugin);
}

static void exit_nuevo(void) {
    /* If the counter is zero, this will be called
    *  We might need to remove /proc info in the future here
    */
    unregister_sched_plugin(&nuevo_plugin);
}

module_init(init_nuevo);
module_exit(exit_nuevo);

