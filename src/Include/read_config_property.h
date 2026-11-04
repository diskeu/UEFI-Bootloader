#ifndef READ_CONFIG_PROPERTY
#define READ_CONFIG_PROPERTY

#include <Uefi.h>
#include <Base.h>

/// @brief Reads a config property from the given file buffer
/// @param key key to read value from
/// @param file_buff pointer to file buffer itself
/// @param file_buff_size size of the buffer
/// @param gBS pointer to BootServices
/// @return value of the key
CHAR8 *read_config_property(
    CONST CHAR8 *key,
    CONST CHAR8 *file_buff,
    CONST UINT64 file_buff_size,
    CONST EFI_BOOT_SERVICES *gBS
);

#endif
