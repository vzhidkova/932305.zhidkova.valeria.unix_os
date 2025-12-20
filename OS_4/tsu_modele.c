#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>

#define PROC_NAME "tsulab"

static int tsulab_show(struct seq_file *m, void *v)
{
    struct timespec64 now;
    time64_t seconds_passed;
    unsigned long long orbits;

    const time64_t sputnik_launch_time = -386380800;
    const unsigned long orbit_period = 5802;

    ktime_get_real_ts64(&now);
    seconds_passed = now.tv_sec - sputnik_launch_time;
    orbits = seconds_passed / orbit_period;

    seq_printf(m,
        "Sputnik-1 orbits around Earth: %llu\n",
        orbits
    );

    return 0;
}

static int tsulab_open(struct inode *inode, struct file *file)
{
    return single_open(file, tsulab_show, NULL);
}

static const struct proc_ops tsulab_ops = {
    .proc_open    = tsulab_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init tsu_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &tsulab_ops);
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tsu_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsu_init);
module_exit(tsu_exit);

MODULE_LICENSE("GPL");