/*
 * These are the E820 types known to the kernel
 */
typedef enum  {

	E820_TYPE_RAM		= 1,
	E820_TYPE_RESERVED	= 2,
	E820_TYPE_ACPI		= 3,
	E820_TYPE_NVS		= 4,
	E820_TYPE_UNUSABLE	= 5,
	E820_TYPE_PMEM		= 7,

	E820_TYPE_PRAM		= 12,

	E820_TYPE_SOFT_RESERVED	= 0xefffffff,

} e820_type;
