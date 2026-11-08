#ifndef GET_MEMORY_MAP
#define GET_MEMORY_MAP

#include <Uefi.h>
#include <Base.h>

/// @brief Retrieves the current UEFI memory map.
/// @param memory_map_size Pointer to the size, in bytes, of the memory map buffer.
/// @param memory_map Pointer to the buffer in which the memory map is returned.
/// @param map_key Pointer to the location in which the memory map key is returned.
/// @param descriptor_size Pointer to the location in which the descriptor size is returned.
/// @param gBS Pointer to BootServices
/// @return Efi status of getting the memory map
EFI_STATUS get_memory_map(
    UINTN                       *memory_map_size,
    EFI_MEMORY_DESCRIPTOR       **memory_map,
    UINTN                       *map_key,
    UINTN                       *descriptor_size,
    EFI_BOOT_SERVICES           *gBS
);

#endif
