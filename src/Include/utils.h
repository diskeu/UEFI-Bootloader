#ifndef UTILS
#define UTILS

#include <Uefi.h>
#include <Base.h>

/// @brief Prints Message with Uefi status
/// @param msg Message to print
/// @param stat Uefi Status to print with the message
void PrintE(CONST CHAR16 *msg, CONST EFI_STATUS stat);


/// @brief Prints Message with \n
/// @param msg Message to print
void Puts(CONST CHAR16 *msg);


/// @brief Converts CHAR8 to CHAR16
/// @param data data to convert
/// @param gBS pointer EfiBootServices
/// @return Message converted in CHAR16
CHAR16 *convert_to_char16(CONST CHAR8 *data, CONST EFI_BOOT_SERVICES *gBS);

#endif
