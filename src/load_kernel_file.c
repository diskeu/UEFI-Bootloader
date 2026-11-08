
#include <Base.h>
#include <Uefi.h>
#include <e820_types.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiLib.h>

#include <bootparam.h>
#include <e820_entry.h>
#include <screen_info.h>

#include "this_handle.h"
#include "utils.h"
#include "read_file.h"
#include "get_memory_map.h"
#include "get_initramfs.h"

#define BZIMAGE_MAGIC_NUMBER 0x53726448
#define PROTOCOL_VERSION 0x20F
#define SETUP_HEADER_OFFSET 0x1F1

__attribute__((optnone))
EFI_STATUS load_kernel_file(
    CHAR8 *kernel_path,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    EFI_BOOT_SERVICES *gBS
) {

    CHAR8 *kernel;
    UINT64 *kernel_buff_size = {0};
    EFI_STATUS stat;

    CHAR16 *kernel_path16 = convert_to_char16(kernel_path, gBS);
    gBS->FreePool(kernel_path);

    // Assuming the conversion failed because of allocation problems
    if (!kernel_path16) {
        return EFI_OUT_OF_RESOURCES;
    }

    // Reading file
    stat = read_file(
        kernel_path16,
        sfsp,
        gBS,
        &kernel,
        kernel_buff_size
    );
    // TODO: Free kernel_path here
    if (EFI_ERROR(stat)) { return stat; }

    UINT32 *magic = (UINT32 *)(kernel+0x0202);

    if (*magic != BZIMAGE_MAGIC_NUMBER) {
        Puts(L"Aborting, the boot protocol version is 'old'");
        return EFI_UNSUPPORTED;
    }
    if (*(UINT16*)(kernel + 0x206) > PROTOCOL_VERSION) {
        Puts(L"Aborting, the boot protocol version is not supported");
        return EFI_UNSUPPORTED;
    }

    /**
     * Allocate memory for boot_params and
     * initalize it to zero
    */
    boot_params *boot_param_buff;

    stat = gBS->AllocatePool(
        EfiLoaderData,
        sizeof(boot_params),
        (VOID**) &boot_param_buff
    );
    if (EFI_ERROR(stat)) { return stat; }

    gBS->SetMem(boot_param_buff, sizeof(*boot_param_buff), 0);

    /**
     * Copy setup header from kernel image into `boot_params`
     */
    gBS->CopyMem(
        boot_param_buff->setup_header,
        (kernel + SETUP_HEADER_OFFSET) /* 0x1F1 specified by `docs.kernel.org` */,
        sizeof(boot_param_buff->setup_header)
    );

    /**
     * Set necessary fields of boot_params->setup_header
     */

    /** Assigned bootloader Id (=Special in our case) */
    UINT8* type_of_loader = (UINT8*) boot_param_buff->setup_header + (0x210 - SETUP_HEADER_OFFSET);
    *type_of_loader = 0xFF;

    /** Indicate that the kernel is loaded above 1mb */
    UINT8* loadflags = (UINT8*) boot_param_buff->setup_header + (0x211 - SETUP_HEADER_OFFSET);
    *loadflags |= 0b00000001;

    /** Set kernel command line pointer to auto */
    static CHAR8 temp[] = "console=ttyS0,115200 earlyprintk=serial,ttyS0,115200 debug";

    UINT32 *cmd_line_ptr = (UINT32*) ((UINT8*) boot_param_buff->setup_header + (0x228 - SETUP_HEADER_OFFSET));
    *cmd_line_ptr = (UINT32) (UINT64) temp; /* Converts CHAR8 * address to 32 bit address  */

    /* Add end bits if address is 64 bit */
    boot_param_buff->ext_cmd_line_ptr = (UINT64) temp > 0xFFFFFFFF ? (UINT32) ((UINT64) temp >> 32) : 0;

    /** Get entry point offset */
    /* get load flags first, configuring the enty points */
    UINT16* xloadflags = (UINT16*) ((UINT8*) boot_param_buff->setup_header + (0x236 - SETUP_HEADER_OFFSET));

    UINT32 offset = 0x200; /* Legacy entry point */

    if ((*xloadflags | 0xFFFE) == 0xFFFE) { /* Bit0 not set, no legacy entry point */

        UINT32* handover_offset = (UINT32*) ((UINT8*) boot_param_buff->setup_header + (0x264 - SETUP_HEADER_OFFSET));

        if ((*xloadflags | 0xFFFB) == 0xFFFF) { /* Bit2 set */
            /* entry point at handover_offset */
            offset = *handover_offset;

        } else {
            /* entry point at handover_offset + 0x200 */
            offset = *handover_offset + 0x200;
        }
    }

    /* Contigious memory the kernel needs */
    UINT32 init_size = *(UINT32*) (kernel + 0x260);

    /* Pages to allocate */
    UINT64 pages = (init_size + 0xFFF) / 0x1000;

    /* Preffered address where the kernel will be loaded */
    EFI_PHYSICAL_ADDRESS pref_address = *(UINT64*) (kernel + 0x258);

    /* Begin of protected mode code */
    EFI_PHYSICAL_ADDRESS load_address = pref_address;

    stat = gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        pages,
        &pref_address
    );

    if (EFI_ERROR(stat)) {
        Print(L"Prefered address - %x not avialable - EFI_STATUS: %d\n", pref_address, stat);

        load_address = 0xFFFFFFFF; /* Under 4GiB */
        
        stat = gBS->AllocatePages(
            AllocateMaxAddress,
            EfiLoaderData,
            pages,
            &load_address
        );

        if (EFI_ERROR(stat)) {
            PrintE(L"Allocating pages under 4Gib failed\n", stat);
            return stat;
        }

        UINT32 kernel_alignement = *(UINT32*) (kernel + 0x230);
        /* round load_address up if its not a multiple of kernel_alignement */
        load_address = (load_address + kernel_alignement - 1) & ~((UINT64) kernel_alignement - 1);
    }
    /* Copy protected mode code to allocated address */
    UINT8 setup_sects = *((UINT8*) kernel + 0x1F1);
    if (setup_sects == 0) setup_sects = 4;

    UINT64 pm_offset = (setup_sects + 1) * 512;

    gBS->CopyMem(
        (VOID*) load_address,
        (VOID*) (kernel + pm_offset),
        *kernel_buff_size - pm_offset
    );

    /* Reading Initial ram filesystem into ram
    and setting corresponding fields in boot_params */
    CHAR8 *initramfs;
    UINTN initramfs_size;

    stat = get_initramfs(
        &initramfs,
        &initramfs_size,
        sfsp,
        gBS
    );
    if (EFI_ERROR(stat)) return stat;

    /* Set the lower 32 bits of the initramfs address */
    *(UINT32*) ((UINT8*) boot_param_buff + 0x218) = (UINT32) (UINT64) initramfs;

    /* Set the lower 32 bits of the initramfs size */
    *(UINT32*) ((UINT8*) boot_param_buff + 0x21C) = (UINT32) initramfs_size;

    /* Setting memory map */
    UINTN                   memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR   *memory_map;
    UINTN                   map_key;
    UINTN                   descriptor_size;

    /* Get memory map */
    stat = get_memory_map(
        &memory_map_size,
        &memory_map,
        &map_key,
        &descriptor_size,
        gBS
    );
    if (EFI_ERROR(stat)) { return stat; }

    /* Iterating through every descriptor */
    EFI_MEMORY_DESCRIPTOR *descriptor;
    UINTN n = 0;
    for (
        ;
        n < 128 && n < memory_map_size / descriptor_size;
        n++
    ) {
        descriptor = (EFI_MEMORY_DESCRIPTOR*) ((UINT8*) memory_map + (n * descriptor_size));

        UINT32 e820_type;

        // get e820_type
        switch (descriptor->Type) {

            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                e820_type = E820_TYPE_RAM;
                break;

            case EfiReservedMemoryType:
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
            case EfiUnusableMemory:
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
            case EfiPalCode:
                e820_type = E820_TYPE_RESERVED;
                break;
            
            case EfiACPIReclaimMemory:
                e820_type = E820_TYPE_ACPI;
                break;

            case EfiACPIMemoryNVS:
                e820_type = E820_TYPE_NVS;
                break;

            case EfiPersistentMemory:
                e820_type = E820_TYPE_PMEM;
                break;
            
            default:
                e820_type = E820_TYPE_RESERVED;
                break;
        }
        
        boot_param_buff->e820_table[n] = (e820entry) {
            .addr = descriptor->PhysicalStart,
            .size = descriptor->NumberOfPages * 4096,
            .type = e820_type
        };
    }
    /* Setting number of entries in e820_entry */
    boot_param_buff->e820_entries = n;

    // Exit Boot services
    stat = gBS->ExitBootServices(this_handle, map_key);
    if (EFI_ERROR(stat)) {
    
        PrintE(L"Couldn't exit boot services", stat);

        /* Still try exiting boot services */

        /* Get memory map */
        stat = get_memory_map(
            &memory_map_size,
            &memory_map,
            &map_key,
            &descriptor_size,
            gBS
        );

        if (EFI_ERROR(stat)) {
            // ...Well...,
            // there is nothing we can do about it...
            PrintE(L"Retry of exiting boot services failed", stat);

            gBS->FreePool(&memory_map);
            return stat;
        }
    }
    /* Kernel entry point */
    UINT64 entry_point = (UINT64) ((UINT8*) load_address + offset);
    __asm__ volatile ("outb %0, %1" :: "a"((UINT8)'X'), "Nd"((UINT16)0x3F8));

    __asm__ __volatile__ (
        "cli\n"             // deactivate interrupts
        "xor %%rdi, %%rdi\n"   // RDI = 0
        "xor %%rbp, %%rbp\n"   // RBP = 0
        "xor %%rbx, %%rbx\n"   // RBX = 0
        "xor %%rdx, %%rdx\n"   // RDX = 0
        "jmp *%1\n"         // jump to kernel entry point
        :
        : "S"(boot_param_buff), "r"(entry_point)
        : "memory"
    );

    return EFI_SUCCESS;
}