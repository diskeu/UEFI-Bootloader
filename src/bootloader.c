// UEFI
#include <Uefi.h>
#include <Base.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiLib.h>

#include <bootparam.h>
#include <e820_entry.h>
#include <screen_info.h>

#include "utils.h"
#include "get_info.h"
#include "get_config_property.h"
#include "read_config_property.h"
#include "read_file.h"
#include "get_memory_map.h"
#include "this_handle.h"
#include "load_kernel_file.h"

EFI_HANDLE this_handle;

__attribute__((noinline))
EFI_STATUS uefi_main(
    EFI_HANDLE imageHandle,
    EFI_SYSTEM_TABLE *systemTable
) {
    Puts(L"Starting bootloader-phase...");

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
        if (sfsp_stat != EFI_SUCCESS) { gBS->Stall(4e6); return sfsp_stat; }
    }

    CHAR8 *kernel_path = NULL;
    EFI_STATUS efi_status;

    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;

    gBS->HandleProtocol(
        imageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImage
    );
    Print(L"ImageBase: %p\n", LoadedImage->ImageBase);

    if (
        !sfsp ||
        !(kernel_path = get_config_property(
            sfsp,
            gBS,
            &efi_status
        ))
    ) {
        Puts(L"Kernel path couldn't be found - Fallback...");
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
        
        if (EFI_ERROR(sfsp_stat)) {
            PrintE(L"Error locating handle buffer", sfsp_stat);
            gBS->Stall(4e6);
            return sfsp_stat;
        }
        
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
                    gBS,
                    &efi_status
                );
                if (kernel_path) { break; }
            }
        }
        gBS->FreePool(handle_buff);
    }
    if (!kernel_path) {
        PrintE(L"Couldn't get kernel path with handle", efi_status);
        gBS->Stall(4e6);
        return efi_status ? efi_status : EFI_NOT_FOUND;
    }

    EFI_STATUS stat = load_kernel_file(kernel_path, sfsp, gBS);
    PrintE(L"load_kernel_file", stat);


    return EFI_ERROR(stat) ? stat : EFI_SUCCESS;
}
