#ifndef READ_FILE
#define READ_FILE

#include <Base.h>
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

/// @brief Reads a file into given file buffer
/// @param path path of the file
/// @param sfsp filesystem to locate file
/// @param gBS pointer to BootServices
/// @param file_buff Filebuffer to read into
/// @param file_buff_size max size of the filebuffer
/// @return EFI_STATUS of reading file
EFI_STATUS read_file(
    CHAR16 *path,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST EFI_BOOT_SERVICES *gBS,
    OUT CHAR8 **file_buff,
    OUT UINT64 *file_buff_size
);

#endif
