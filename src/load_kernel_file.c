
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

#define BZIMAGE_MAGIC_NUMBER 0x53726448
#define PROTOCOL_VERSION 0x20F
#define SETUP_HEADER_OFFSET 0x1F1

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
    static CHAR8 temp[] = "console=tty0 earlyprintk=efi,keep keep_bootcon";

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

    /* Setting memory map */
    UINTN                   memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR   memory_map;
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

    /* Setting number of entries in e820_entry */
    boot_param_buff->e820_entries = memory_map_size / sizeof(memory_map);

    /* Iterating through every descriptor */
    for (
        EFI_MEMORY_DESCRIPTOR *descriptor = &memory_map;
        descriptor - &memory_map < boot_param_buff->e820_entries;
        descriptor++
    ) {

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
        
        boot_param_buff->e820_table[
            BOOT_E820_ENTRY_SIZE * (descriptor - &memory_map)
        ] = (e820entry) {
            .addr = descriptor->PhysicalStart,
            .size = descriptor->NumberOfPages * 4096,
            .type = e820_type
        };
    }

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
    UINT64 entry_point = (UINT64) ((UINT8*) kernel + offset);

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