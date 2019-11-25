#include <linux/module.h>
#include <litmus/preempt.h>
#include <litmus/sched_plugin.h>
#include <litmus/litmus.h>      /* API definitions       */
#include <litmus/jobs.h>
#include <litmus/budget.h>
#include <linux/percpu.h>	/* CPU-local allocations */
#include <linux/sched.h>	/* struct task_struct    */
#include <linux/list.h>

#define MODULE_NAME         "MI_PLUGIN"

/* Task (node) */
struct task_item {
	struct task_struct* item;
	struct list_head links;
};

struct nuevo_cpu_state {
	int cpu;
	struct task_struct* scheduled;
	/* FCFS-queue ghost node*/
	struct list_head ghost_node;
};

static DEFINE_PER_CPU(struct nuevo_cpu_state, nuevo_cpu_state);

#define cpu_state_for(cpu_id)	(&per_cpu(nuevo_cpu_state, cpu_id))

static long nuevo_activate_plugin(void) {

	int cpu;
	struct nuevo_cpu_state *state;

	for_each_online_cpu() {
		printk(KERN_INFO "Initializing CPU %d...\n",cpu);
		state = cpu_state_for(cpu);
		state->cpu = cpu;
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

	/* Mandatory. Triggers transition in the LITMUS remote preemption
         * state machine. Has to be called after a local scheduling decision
         */
	sched_state_task_picked();

   return NULL;
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

    /* INIT_LIST_HEAD(&ghost_node);
       maybe at the activate ?? Otherwise only once initialized
    */
    printk(KERN_INFO "[0] Module %s loaded \n", MODULE_NAME);

  return register_sched_plugin(&nuevo_plugin);
}

static void exit_nuevo(void) {
    /* If the counter is zero, this will be called (CREO)
    *  We might need to remove /proc info in the future here
    */
    unregister_sched_plugin(&nuevo_plugin);
}

module_init(init_nuevo);
module_exit(exit_nuevo);

