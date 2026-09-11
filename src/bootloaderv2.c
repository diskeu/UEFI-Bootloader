// UEFI
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiLib.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>

#include <bootparam.h>
#include <e820_entry.h>
#include <e820_types.h>

#define BOOT_CONFIG_PATH L"\\boot\\config"
#define KERNEL_PATH_KEY "kernel"
#define CONFIG_LINE_SIZE (256)


static EFI_HANDLE this_handle;


typedef enum {
    KEY,
    VALUE,
    MV_UNTIL_NEXT_KEY
} ReadingState;


static void PrintE(CONST CHAR16 *msg, CONST EFI_STATUS stat) {
    Print(msg);
    Print(L"    |EFI_STATUS: %llu|\n", stat);
}


CHAR16 *convert_to_char16(CONST CHAR8 *data, CONST EFI_BOOT_SERVICES *gBS) {
    CONST CHAR8 *ptr = data;
    CHAR16 *result;

    EFI_STATUS stat = gBS->AllocatePool(
        EfiLoaderData,
        AsciiStrLen(data)+1,
        (VOID**) &result
    );
    if (EFI_ERROR(stat)) { return NULL; }
    
    while (*ptr != '\0') {
        *result = (CHAR16) *ptr;
        ptr++; result++;
    }
    *result = L'\0';

    return result;
}


EFI_STATUS get_info(
    EFI_FILE_PROTOCOL *sub,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_FILE_INFO **info
) {
    UINTN buff_size     = 0;
    EFI_STATUS stat;

    sub->GetInfo(
        sub,
        &gEfiFileInfoGuid,
        &buff_size,
        NULL
    );

    stat = gBS->AllocatePool(
        EfiLoaderData,
        buff_size,
        (VOID**) info
    );

    if (EFI_ERROR(stat)) {
        PrintE(L"Error while allocating info buffer", stat);
        return stat;
    }

    stat = sub->GetInfo(
        sub,
        &gEfiFileInfoGuid,
        &buff_size,
        (VOID**) info
    );

    if (EFI_ERROR(stat)) {
        PrintE(L"Error Getting file info", stat);
        gBS->FreePool(info);
        return stat;
    }
    return EFI_SUCCESS;
}


EFI_STATUS read_file(
    CHAR16 *path,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST EFI_BOOT_SERVICES *gBS,
    OUT CHAR8 **file_buff,
    OUT UINT64 *file_buff_size
) {
    // Opening root-directory
    EFI_FILE_PROTOCOL *root;
    EFI_STATUS         stat;

    stat = sfsp->OpenVolume(sfsp, &root);
    if (EFI_ERROR(stat)) {
        PrintE(L"Error opening volume", stat);
        return stat;
    }

    // Opening path
    EFI_FILE_PROTOCOL *sub;

    stat = root->Open(
        root,
        &sub,
        path,
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error opening file location", stat);
        return stat;
    }
    // Getting file info
    EFI_FILE_INFO *info;

    stat = get_info(sub, gBS, &info);
    if (EFI_ERROR(stat)) { return stat; }

    // Allocating file buffer
    *file_buff_size = info->FileSize; 

    gBS->FreePool(info);

    stat = gBS->AllocatePool(
        EfiLoaderData,
        *file_buff_size,
        (VOID**) file_buff
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error allocating file buffer", stat);
        return stat;
    }

    // Reading into file buffer
    stat = sub->Read(
        sub,
        file_buff_size,
        (VOID**) file_buff
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error while reading into file buffer", stat);
        gBS->FreePool(*file_buff);
        return stat;
    } 

    return EFI_SUCCESS;
}


CHAR8 *read_config_property(
    CONST CHAR8 *key,
    CONST CHAR8 *file_buff,
    CONST UINT64 file_buff_size,
    CONST EFI_BOOT_SERVICES *gBS
) {
    CONST CHAR8 *read_pos      = file_buff;
    ReadingState read_st       = KEY;
    short k_buff_i             = 0,
          v_buff_i             = 0;
    CHAR8 k_buff[CONFIG_LINE_SIZE+1];
    CHAR8 *v_buff;

    EFI_STATUS alloc_stat = gBS->AllocatePool(
        EfiLoaderData,
        CONFIG_LINE_SIZE+1,
        (VOID**) &v_buff
    );
    if (EFI_ERROR(alloc_stat)) { return NULL; }

    while (read_pos - file_buff < file_buff_size) {

        switch (read_st) {

            case MV_UNTIL_NEXT_KEY:
                if (*read_pos == '\n') {
                    read_st = KEY;
                } else {
                    read_pos++;
                }
                break;

            case KEY:
                if (
                    *read_pos != '#' ||
                    *read_pos != '=' ||
                    *read_pos != ' '
                ) {

                }
                switch (*read_pos) {

                    case '#':
                        read_st = MV_UNTIL_NEXT_KEY;
                        break;
                    
                    case '=':
                        k_buff[k_buff_i] = '\0';
                        if (AsciiStrCmp(key, k_buff)) {
                            read_st = VALUE;
                            break;
                        }
                        k_buff_i = 0;
                        read_st  = MV_UNTIL_NEXT_KEY;
                        break;
                    
                    case ' ': break;

                    default:
                        k_buff[k_buff_i++] = *read_pos;
                        
                }

            case VALUE:
                if (
                    *read_pos == '#' ||
                    *read_pos == '\r' ||
                    *read_pos == '\n'
                ) {
                    v_buff[v_buff_i] = '\0';
                    return v_buff;
                }
                if (*read_pos != ' ') { v_buff[v_buff_i++] = *read_pos; }
                break;
        }
    }
    return NULL;
}


CHAR8 * get_config_property(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST CHAR8 *key,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_STATUS *efi_status
) {
    CHAR8 *file_buff;
    UINT64 file_buff_size;
    *efi_status = EFI_SUCCESS;

    *efi_status = read_file(
        BOOT_CONFIG_PATH,
        sfsp,
        gBS,
        &file_buff,
        &file_buff_size
    );
    if (EFI_ERROR(*efi_status)) { return NULL; }

    CHAR8 *kernel_path = read_config_property(
        KERNEL_PATH_KEY,
        file_buff,
        file_buff_size,
        gBS
    );
    gBS->FreePool(file_buff);

    if (!kernel_path) {
        Print(
            L"Couldn't find correct key in boot config. Kernel path key: |%a|\n",
            KERNEL_PATH_KEY
        );
        *efi_status = EFI_NOT_FOUND;
        gBS->FreePool(kernel_path);
        return NULL;
    }

    return kernel_path;
}


EFI_STATUS get_memory_map(
    UINTN                       *memory_map_size,
    EFI_MEMORY_DESCRIPTOR       *memory_map,
    UINTN                       *map_key,
    UINTN                       *descriptor_size,
    EFI_BOOT_SERVICES           *gBS
) {

    memory_map_size = 0;
    memory_map = NULL;
    UINT32                  descriptor_version;
    EFI_STATUS              stat;

    /**
     * One time finding memory_map_size and
     * one time actually getting the results
     */
    stat = gBS->GetMemoryMap(
        memory_map_size,
        memory_map,
        map_key,
        descriptor_size,
        &descriptor_version
    );
    if (EFI_ERROR(stat) && stat != EFI_BUFFER_TOO_SMALL) { return stat; }

    stat = gBS->AllocatePool(
        EfiLoaderData,
        *memory_map_size,
        (VOID**) &memory_map
    );
    if (EFI_ERROR(stat)) { return stat; }

    /* Call memory map with correct buffer size */
    stat = gBS->GetMemoryMap(
        memory_map_size,
        memory_map,
        map_key,
        descriptor_size,
        &descriptor_version
    );
    return EFI_ERROR(stat) ? stat : EFI_SUCCESS;
}


#define BZIMAGE_MAGIC_NUMBER 0x53647248
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

    // Assuming the conversion failed because of allocation problems
    CHAR16 *kernel_path16;
    if (
        !(kernel_path16 = convert_to_char16(kernel_path, gBS))
    ) {
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
    if (EFI_ERROR(stat)) { return stat; }

    UINT32 *magic = (UINT32 *)(kernel+0x0202);

    if (*magic != BZIMAGE_MAGIC_NUMBER) {
        Print(L"Aborting, the boot protocol version is 'old'");
        return EFI_UNSUPPORTED;
    }
    if (*(UINT16*)(kernel + 0x206) > PROTOCOL_VERSION) {
        Print(L"Aborting, the boot protocol version is not supported");
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
    UINT8* type_of_loader = ((UINT8*) boot_param_buff) + 0x210;
    *type_of_loader = 0xFF;

    /** Indicate that the kernel is loaded above 1mb */
    UINT8* loadflags = ((UINT8*) boot_param_buff) + 0x211;
    *loadflags |= 0b00000001;

    /** Set kernel command line pointer to auto */
    CHAR8 *temp = "auto";

    UINT32 *cmd_line_ptr = (UINT32*) (((UINT8*) boot_param_buff) + 0x228);
    *cmd_line_ptr = (UINT32) (UINT64) temp; /* Converts CHAR8 * address to 32 bit address  */

    /* Add end bits if address is 64 bit */
    boot_param_buff->ext_cmd_line_ptr = (UINT64) temp > 0xFFFFFFFF ? (UINT32) ((UINT64) temp >> 32) : 0;

    /* Setting memory map */
    UINTN                   memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR   *memory_map = {0};
    UINTN                   map_key;
    UINTN                   descriptor_size;

    /* Get memory map */
    stat = get_memory_map(
        &memory_map_size,
        memory_map,
        &map_key,
        &descriptor_size,
        gBS
    );
    if (EFI_ERROR(stat)) { return stat; }

    /* Setting number of entries in e820_entry */
    boot_param_buff->e820_entries = memory_map_size / sizeof(memory_map);

    /* Iterating through every descriptor */
    for (
        EFI_MEMORY_DESCRIPTOR *descriptor = memory_map;
        descriptor - memory_map < boot_param_buff->e820_entries;
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
            BOOT_E820_ENTRY_SIZE * (descriptor - memory_map)
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
            memory_map,
            &map_key,
            &descriptor_size,
            gBS
        );

        if (EFI_ERROR(stat)) {
            // ...Well...,
            // there is nothing we can do about it...
            PrintE(L"Retry of exiting boot services failed", stat);

            gBS->FreePool((VOID*) memory_map);
            return stat;
        }
    }
    
    /* Kernel entry point */
    UINT64 entry_point = (UINT64) ((UINT8*) kernel + 0x200);

    /* Timeout to view cli logs */
    Print("Timeout before JMP...");
    gBS->Stall(4e6);

    __asm__ __volatile__ (
        "cli\n"             // deactivate interrupts
        "mov %0, %%rsi\n"   // RSI = boot_params
        "xor $0, %%rdi\n"   // RDI = 0
        "xor $0, %%rbp\n"   // RBP = 0
        "xor $0, %%rbx\n"   // RBX = 0
        "xor $0, %%rdx\n"   // RDX = 0
        "jmp *%1\n"         // jump to kernel entry point
        :
        : "r"(boot_param_buff), "r"(entry_point)
        : "rsi", "rdi", "rbp", "rbx", "rdx" 
    );

    return EFI_SUCCESS;
}


EFI_STATUS uefi_main(
    EFI_HANDLE imageHandle,
    EFI_SYSTEM_TABLE *systemTable
) {
    // Set global imageHandle
    this_handle = imageHandle;

    // OpenProtocol changes the `loaded_image` pointer itself to the protocol
    EFI_LOADED_IMAGE_PROTOCOL       *loaded_image;
    EFI_STATUS                       sfsp_stat;
    EFI_STATUS                       loaded_img_stat;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = NULL;
    EFI_BOOT_SERVICES                *gBS = systemTable->BootServices;

    loaded_img_stat = gBS->OpenProtocol(
        imageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**) &loaded_image,
        imageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (loaded_img_stat == EFI_SUCCESS) {
        EFI_HANDLE handle = loaded_image->DeviceHandle;

        EFI_STATUS sfsp_stat = gBS->OpenProtocol(
            handle,
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID**) &sfsp,
            imageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
        if (sfsp_stat != EFI_SUCCESS) { return sfsp_stat; }
    }

    CHAR8 *kernel_path = NULL;
    EFI_STATUS *efi_status = NULL;

    if (
        !sfsp ||
        !(kernel_path = get_config_property(
            sfsp,
            "kernel",
            gBS,
            efi_status
        ))
    ) {
        // Fallback
        EFI_HANDLE *handle_buff;
        UINTN handle_count = 0;

        sfsp_stat = gBS->LocateHandleBuffer(
            ByProtocol,
            &gEfiSimpleFileSystemProtocolGuid,
            NULL,
            &handle_count,
            &handle_buff
        );
        
        if (EFI_ERROR(sfsp_stat)) { return sfsp_stat; }
        
        for (
            EFI_HANDLE *handle = handle_buff;
            handle - handle_buff < handle_count;
            handle++
        ) {
            if (
                gBS->OpenProtocol(
                    handle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID**) &sfsp,
                    imageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                ) == EFI_SUCCESS
            ) {
                kernel_path = get_config_property(
                    sfsp,
                    "kernel",
                    gBS,
                    efi_status
                );
                if (kernel_path) { break; }
            }
        }
        gBS->FreePool(handle_buff);
    }
    if (!kernel_path) {
        PrintE(L"Couldn't get kernel path with handle", *efi_status);
        return *efi_status ? *efi_status : EFI_NOT_FOUND;
    }
    gBS->FreePool(kernel_path);

    EFI_STATUS stat = load_kernel_file(kernel_path, sfsp, gBS);

    /* Timeout to view cli logs */
    Print("Timeout...");
    gBS->Stall(4e6);

    return EFI_ERROR(stat) ? stat : EFI_SUCCESS;
}
