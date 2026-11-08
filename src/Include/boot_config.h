// Defines settings for boot config file
#ifndef BOOT_CONFIG
#define BOOT_CONFIG

// Defines path where boot config file is located
#define BOOT_CONFIG_PATH L"\\boot\\config"
// Defines path where the initial ram filesystem is located
#define INIT_RAMFS_PATH L"\\boot\\corepure64.gz"
// Defines key to get value from in boot config file
#define KERNEL_PATH_KEY "kernel"
// Defines the max size if a single line in the boot config file
#define CONFIG_LINE_SIZE (256)

#endif
