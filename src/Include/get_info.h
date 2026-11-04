#ifndef GET_INFO
#define GET_INFO

#include <Uefi.h>
#include <Base.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

/// @brief Gets file info from a given file protocol
/// @param sub file protocol to get file inf ofrom
/// @param gBS pointer to BootServices
/// @param info pointer to pointer to an empty EFI_FILE_INFO
/// @return EFI_STATUS of getting file info
EFI_STATUS get_info(
    EFI_FILE_PROTOCOL *sub,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_FILE_INFO **info
);

#endif
