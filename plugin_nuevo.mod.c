#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x5f1f6366, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xdab9f533, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x35a67c4c, __VMLINUX_SYMBOL_STR(unregister_sched_plugin) },
	{ 0xa7ca0a55, __VMLINUX_SYMBOL_STR(register_sched_plugin) },
	{ 0x3476500c, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x8b775b7d, __VMLINUX_SYMBOL_STR(resched_state) },
	{ 0xc917e655, __VMLINUX_SYMBOL_STR(debug_smp_processor_id) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0xc671e369, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x88db9f48, __VMLINUX_SYMBOL_STR(__check_object_size) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x343a1a8, __VMLINUX_SYMBOL_STR(__list_add) },
	{ 0x18ec45dd, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x7485b066, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0x328b2dbc, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0x521445b, __VMLINUX_SYMBOL_STR(list_del) },
	{ 0xfe7c4287, __VMLINUX_SYMBOL_STR(nr_cpu_ids) },
	{ 0xc0a3d105, __VMLINUX_SYMBOL_STR(find_next_bit) },
	{ 0x477e59a3, __VMLINUX_SYMBOL_STR(__cpu_online_mask) },
	{ 0x21f3954b, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x1916e38c, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_irqrestore) },
	{ 0x680ec266, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irqsave) },
	{ 0x5ecfeec6, __VMLINUX_SYMBOL_STR(__per_cpu_offset) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

