#include <Base.h>
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include "utils.h"
#include "get_info.h"

EFI_STATUS read_file(
    CHAR16 *path,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST EFI_BOOT_SERVICES *gBS,
    OUT CHAR8 **file_buff,
    OUT UINT64 *file_buff_size,
    UINT8 load_under4gib
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
        if (stat == EFI_NOT_FOUND) { Puts(L"File does not exist"); }
        return stat;
    }
    // Getting file info
    EFI_FILE_INFO *info;

    stat = get_info(sub, gBS, &info);
    if (EFI_ERROR(stat)) { return stat; }

    *file_buff_size = info->FileSize; 
    gBS->FreePool(info);

    // Allocating file buffer
    EFI_PHYSICAL_ADDRESS addr;
    UINT64 pages = (*file_buff_size + 0xFFF) / 0x1000;

    if (!load_under4gib) {
        addr = 0;
        stat = gBS->AllocatePages(
            AllocateAnyPages,
            EfiLoaderData,
            pages,
            &addr
        );
    } else {
        addr = 0xFFFFFFFF;
        stat = gBS->AllocatePages(
            AllocateMaxAddress,
            EfiLoaderData,
            pages,
            &addr
        );
    }
    if (EFI_ERROR(stat)) {
        PrintE(L"Error allocating file buffer", stat);
        return stat;
    }
    *file_buff = (CHAR8*) (UINT64) addr;

    // Reading into file buffer
    stat = sub->Read(
        sub,
        file_buff_size,
        *file_buff
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Error while reading into file buffer", stat);
        gBS->FreePages(addr, pages);
        return stat;
    } 

    return EFI_SUCCESS;
}
