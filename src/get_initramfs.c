#include <Uefi.h>
#include <Base.h>

#include "boot_config.h"
#include "read_file.h"
#include "utils.h"

EFI_STATUS get_initramfs(
    CHAR8 **initramfs,
    UINTN *initramfs_size,
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp,
    EFI_BOOT_SERVICES *gBS
) {
    EFI_STATUS stat;

    stat = read_file(
        INIT_RAMFS_PATH,
        sfsp,
        gBS,
        initramfs,
        initramfs_size,
        1
    );
    if (EFI_ERROR(stat)) {
        PrintE(L"Couldn't get initial ram filesystem", stat);
    }
    return stat;
}
