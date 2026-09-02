#ifndef BOOTPARAM
#define BOOTPARAM

#include <Base.h>
#include <e820_entry.h>

#define EDDMAXNR 6
#define EDD_MBR_SIG_MAX 2
#define E820_MAX_ENTRIES_ZEROPAGE 128
#define BOOT_E820_ENTRY_SIZE 20 /* sizeof(boot_e820_entry) */

struct boot_params {
	UINT8 screen_info[0x040]; /* 0x000 */
	UINT8 apm_bios_info[0x054 - 0x040]; /* 0x040 */
	UINT8  _pad2[4];					/* 0x054 */
	UINT64  tboot_addr;				/* 0x058 */
	UINT8 ist_info[0x070 - 0x060]; /* 0x060 */
	UINT64 acpi_rsdp_addr;				/* 0x070 */
	UINT8  _pad3[8];					/* 0x078 */
	UINT8  hd0_info[16];	/* obsolete! */		/* 0x080 */
	UINT8  hd1_info[16];	/* obsolete! */		/* 0x090 */
	UINT8 sys_desc_table[0x0b0 - 0x0a0]; /* obsolete! */	/* 0x0a0 */
	UINT8 olpc_ofw_header[0x0c0 - 0x0b0];		/* 0x0b0 */
	UINT32 ext_ramdisk_image;			/* 0x0c0 */
	UINT32 ext_ramdisk_size;				/* 0x0c4 */
	UINT32 ext_cmd_line_ptr;				/* 0x0c8 */
	UINT8  _pad4[112];				/* 0x0cc */
	UINT32 cc_blob_address;			/* 0x13c */
	UINT8 edid_info[0x1c0 - 0x140];	    /* 0x140 */
	UINT8 efi_info[0x1e0 - 0x1c0];		/* 0x1c0 */
	UINT32 alt_mem_k;   /* 0x1e0 */
	UINT32 scratch;		/* Scratch field! */	/* 0x1e4 */
	UINT8  e820_entries;				/* 0x1e8 */
	UINT8  eddbuf_entries;				/* 0x1e9 */
	UINT8  edd_mbr_sig_buf_entries;			/* 0x1ea */
	UINT8  kbd_status;				/* 0x1eb */
	UINT8  secure_boot;				/* 0x1ec */
	UINT8  _pad5[2];					/* 0x1ed */
    /**
     * Sentinel check for bootloader. If more then setup_header is
     * copied sentinel 'll be 0x0ff and not zero
     */
	UINT8  sentinel;					/* 0x1ef */
	UINT8  _pad6[1];					/* 0x1f0 */
	UINT8 setup_header[0x290 - 0x1f1];    /* setup header */	/* 0x1f1 */
    /**
     * Not used due having setup_header as fixed size
    */
	// UINT8  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
	UINT32 edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
	e820entry e820_table[E820_MAX_ENTRIES_ZEROPAGE]; /* 0x2d0 */
	UINT8  _pad8[48];				/* 0xcd0 */
	UINT8 edd_info[(0x0d00 - 0xcd0) / EDDMAXNR];	/* 64 bytes*/	/* 0xd00 */
	UINT8  _pad9[276];				/* 0xeec */
} __attribute__((packed));

#endif