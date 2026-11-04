#include <Base.h>
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include "utils.h"

EFI_STATUS get_info(
    EFI_FILE_PROTOCOL *sub,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_FILE_INFO **info
) {
    Puts(L"get_info"); /* Temp debug */
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
        *info
    );

    if (EFI_ERROR(stat)) {
        PrintE(L"Error Getting file info", stat);
        gBS->FreePool(info);
        return stat;
    }
    return EFI_SUCCESS;
}
