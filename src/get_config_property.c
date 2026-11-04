#include <Base.h>
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiLib.h>

#include "boot_config.h"
#include "utils.h"
#include "read_file.h"
#include "read_config_property.h"

CHAR8 * get_config_property(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    CONST EFI_BOOT_SERVICES *gBS,
    EFI_STATUS *efi_status
) {
    Puts(L"get_config_property"); /* Temp debug */

    CHAR8 *file_buff;
    UINT64 file_buff_size;
    *efi_status = EFI_SUCCESS;

    *efi_status = read_file(
        BOOT_CONFIG_PATH,
        sfsp,
        gBS,
        &file_buff,
        &file_buff_size
    );
    if (EFI_ERROR(*efi_status)) { return NULL; }

    CHAR8 *kernel_path = read_config_property(
        KERNEL_PATH_KEY,
        file_buff,
        file_buff_size,
        gBS
    );
    gBS->FreePool(file_buff);

    if (!kernel_path) {
        Print(
            L"Couldn't find correct key in boot config. Kernel path key: |%a|\n",
            KERNEL_PATH_KEY
        );
        *efi_status = EFI_NOT_FOUND;
        gBS->FreePool(kernel_path);
        return NULL;
    }
    return kernel_path;
}