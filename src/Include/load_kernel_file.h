#ifndef LOAD_KERNEL_FILE
#define LOAD_KERNEL_FILE

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

/// @brief reads file of kernel path, sets up configuration and
///        finally jumps to it's entry point.
/// @param kernel_path filepath of the kernel file
/// @param sfsp filesystem where kernel image is located on
/// @param gBS pointer to BootServices
/// @return Efi status of loading kernel file
EFI_STATUS load_kernel_file(
    CHAR8 *kernel_path,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    EFI_BOOT_SERVICES *gBS
);

#endif
