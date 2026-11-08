#include <Base.h>
#include <Uefi.h>
#include "utils.h"

EFI_STATUS get_memory_map(
    UINTN                       *memory_map_size,
    EFI_MEMORY_DESCRIPTOR       **memory_map,
    UINTN                       *map_key,
    UINTN                       *descriptor_size,
    EFI_BOOT_SERVICES           *gBS
) {
    /* Since a call to print can change the memory map, there
    will be no print debug statements if no known error occures */
    Puts(L"Getting memory map. Print statements are now disabled.");

    UINT32                  descriptor_version;
    EFI_STATUS              stat;
    *memory_map_size = 0;

    /**
     * One time finding memory_map_size and
     * one time actually getting the results
     */
    stat = gBS->GetMemoryMap(
        memory_map_size,
        NULL,
        map_key,
        descriptor_size,
        &descriptor_version
    );
    if (EFI_ERROR(stat) && stat != EFI_BUFFER_TOO_SMALL) { return stat; }

    /* Add extra space for the allocation below */
    *memory_map_size += *descriptor_size * 2;

    stat = gBS->AllocatePool(
        EfiLoaderData,
        *memory_map_size,
        (VOID**) memory_map
    );
    if (EFI_ERROR(stat)) { return stat; }

    /* Call memory map with correct buffer size */
    stat = gBS->GetMemoryMap(
        memory_map_size,
        *memory_map,
        map_key,
        descriptor_size,
        &descriptor_version
    );
    return EFI_ERROR(stat) ? stat : EFI_SUCCESS;
}
