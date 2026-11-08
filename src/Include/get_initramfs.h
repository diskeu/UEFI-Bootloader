#ifndef GET_INITRAMFS
#define GET_INITRAMFS

#include <Uefi.h>
#include <Base.h>
#include <Protocol/SimpleFileSystem.h>

/// @brief Loads initalramfs into ram.
/// @param initramfs file pointer
/// @param initramfs_size size of initalramfs
/// @param sfsp pointer to filesystem
/// @param gBS pointer to BootServices
/// @return status of getting initalramfs
EFI_STATUS get_initramfs(
    CHAR8 **initramfs,
    UINTN *initramfs_size,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    EFI_BOOT_SERVICES *gBS
);

#endif