// UEFI
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiLib.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>

#define BOOT_CONFIG_PATH L"\\boot\\config"
#define KERNEL_PATH_KEY "kernel"
#define CONFIG_LINE_SIZE (256)

typedef enum {
    KEY,
    VALUE,
    MV_UNTIL_NEXT_KEY
} ReadingState;

static void PrintE(CONST CHAR16 *msg, CONST EFI_STATUS stat) {
    Print(msg);
    Print(L"    |EFI_STATUS: %llu|\n", stat);
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

    gBS->AllocatePool(
        EfiLoaderData,
        CONFIG_LINE_SIZE+1,
        (VOID**) &v_buff
    );
    if (EFI_ERROR(v_buff)) {
        gBS->FreePool(v_buff);
        return NULL;
    }

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
    CONST char *key,
    CHAR16 *path,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_STATUS *efi_status
) {

    EFI_FILE_PROTOCOL *root;
    EFI_STATUS         stat;

    if (
        (stat = sfsp->OpenVolume(sfsp, &root))
        != EFI_SUCCESS
    ) {
        PrintE(L"Error opening volume", stat);
        *efi_status = stat;
        return NULL;
    }

    EFI_FILE_PROTOCOL *sub;

    stat = root->Open(
        root,
        &sub,
        path,
        EFI_FILE_MODE_READ,
        0
    );

    if (EFI_ERROR(stat)) {
        PrintE(L"Error opening config file location", stat);
        *efi_status = stat;
        return NULL;
    }

    EFI_FILE_INFO *info = NULL;
    UINTN buff_size     = 0;

    sub->GetInfo(
        sub,
        &gEfiFileInfoGuid,
        &buff_size,
        NULL
    );

    stat = gBS->AllocatePool(
        EfiLoaderData,
        buff_size,
        (VOID**)&info
    );

    if (EFI_ERROR(stat)) {
        PrintE(L"Error while allocating info buffer", stat);
        *efi_status = stat;
        return NULL;
    }

    stat = sub->GetInfo(
        sub,
        &gEfiFileInfoGuid,
        &buff_size,
        (VOID**) &info
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error Getting file info", stat);
        *efi_status = stat;
        return NULL;
    }

    CHAR8 *file_buff;
    UINT64 file_buff_size = info->FileSize; 

    stat = gBS->AllocatePool(
        EfiLoaderData,
        file_buff_size,
        (VOID**) &file_buff
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error allocating file buffer", stat);
        *efi_status = stat;
        return NULL;
    }

    stat = sub->Read(
        sub,
        &file_buff_size,
        (VOID**) &file_buff
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error while reading into file buffer", stat);
        *efi_status = stat;
        return NULL;
    }

    CHAR8 *kernel_path = read_config_property(
        KERNEL_PATH_KEY,
        file_buff,
        file_buff_size,
        gBS
    );

    if (!kernel_path) {
        Print(
            L"Couldn't find correct key in boot config. Kernel path key: |%a|\n",
            KERNEL_PATH_KEY
        );
        *efi_status = stat;
        return NULL;
    }
    gBS->FreePool(kernel_path);

    return kernel_path;
}

EFI_STATUS uefi_main(
    EFI_HANDLE imageHandle,
    EFI_SYSTEM_TABLE *systemTable
) {

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
            BOOT_CONFIG_PATH,
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
                    BOOT_CONFIG_PATH,
                    gBS,
                    efi_status
                );
                if (kernel_path) { break; }
            }
        }
    }
    if (!kernel_path) {
        PrintE(L"Couldn't get kernel path with handle", *efi_status);
        return *efi_status ? *efi_status : EFI_NOT_FOUND;
    }
    return EFI_SUCCESS;
}
