#include <linux/init.h>
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/module.h> /* Needed by all modules */
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/tracepoint.h>
#include <trace/events/sched.h>

MODULE_LICENSE("GPL");

static const size_t MAX_STR_LENGTH = 15;
#define PROC_NAME "bash"
#define TRACEPOINT_PROBE(probe, args...) static void probe(void* __data, args)

static bool check_proc_name(char* proc_name, struct task_struct* task)
{
    char* task_name;
    int cmp_max_len;
    
    if (task == NULL) {
        task = current;
    }
    task_name = task->comm;
    cmp_max_len = min(strlen(proc_name), MAX_STR_LENGTH);
    return strncmp(task_name, proc_name, cmp_max_len) == 0;
}

TRACEPOINT_PROBE(sched_switch_probe, bool preemt, struct task_struct* prev, struct task_struct* next);

TRACEPOINT_PROBE(sched_switch_probe, bool preemt, struct task_struct* prev, struct task_struct* next)
{
    if (check_proc_name(PROC_NAME, prev)) {
        printk(KERN_INFO "Schedule to %s \n", next->comm);
    }
}

struct tracepoints_table {
    const char* name;
    void* fct;
    struct tracepoint* value;
    char init;
};

struct tracepoints_table interests[] = {
    // {.name = "sys_enter", .fct = syscall_enter_probe},
    // {.name = "sys_exit", .fct = syscall_exit_probe},
    { .name = "sched_switch", .fct = sched_switch_probe },
};

#define FOR_EACH_INTEREST(i) \
    for (i = 0; i < sizeof(interests) / sizeof(struct tracepoints_table); i++)

/**
 * Tracepoints are not exported (see
 * http://lkml.iu.edu/hypermail/linux/kernel/1504.3/01878.html).
 * This function find the struct tracepoint* associated with a given tracepoint
 * name.
 */
static void lookup_tracepoints(struct tracepoint* tp, void* ignore)
{
    int i;
    FOR_EACH_INTEREST(i)
    {
        if (strcmp(interests[i].name, tp->name) == 0)
            interests[i].value = tp;
    }
}

static void cleanup(void)
{
    int i;

    // Cleanup the tracepoints
    FOR_EACH_INTEREST(i)
    {
        if (interests[i].init) {
            tracepoint_probe_unregister(interests[i].value, interests[i].fct, NULL);
            tracepoint_synchronize_unregister();
        }
    }
}

static void register_hooks(void)
{
    // Install the tracepoints
    int i;
    for_each_kernel_tracepoint(lookup_tracepoints, NULL);

    FOR_EACH_INTEREST(i)
    {
        if (interests[i].value == NULL) {
            printk("Error, %s not found\n", interests[i].name);
            // Unload previously loaded
            cleanup();
        }
        interests[i].init = 1;
        tracepoint_probe_register(interests[i].value, interests[i].fct, NULL);
    }
}

static int __init lkm_hello_init(void)
{
    printk(KERN_INFO "Hello World!\n");
    register_hooks();
    // preempt_notifier_init(&notifier, &ops);
    return 0;
}

static void __exit lkm_hello_exit(void)
{
    cleanup();
    printk(KERN_INFO "Goodbye!\n");
}

module_init(lkm_hello_init);
module_exit(lkm_hello_exit);