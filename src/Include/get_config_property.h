#ifndef GET_CONFIG_PROPERTY
#define GET_CONFIG_PROPERTY

#include <Uefi.h>
#include <Base.h>
#include <Protocol/SimpleFileSystem.h>

/// @brief Gets a config property
/// @param sfsp Filesystem to locate boot config file from
/// @param gBS pointer to BootServices
/// @param efi_status Pointer to efi status
/// @return Value of the config property
CHAR8 * get_config_property(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_STATUS *efi_status
);

#endif
