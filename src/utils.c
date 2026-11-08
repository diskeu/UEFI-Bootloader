
#include <Uefi.h>
#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <stddef.h>

void PrintE(CONST CHAR16 *msg, CONST EFI_STATUS stat) {
    Print(msg);
    Print(L"    |EFI_STATUS: %llu|\n", stat);
}


void Puts(CONST CHAR16 *msg) {
    Print(msg); Print(L"\n");
}


CHAR16 *convert_to_char16(CONST CHAR8 *data, CONST EFI_BOOT_SERVICES *gBS) {

    UINTN len = AsciiStrLen(data);
    CHAR16 *result;
    CHAR16 *ptr;

    EFI_STATUS stat = gBS->AllocatePool(
        EfiLoaderData,
        (len + 1) * sizeof(CHAR16),
        (VOID **)&result
    );

    if (EFI_ERROR(stat)) {
        return NULL;
    }
    ptr = result;

    while (*data != '\0') {
        *ptr = (CHAR16)*data;
        data++; ptr++;
    }
    *ptr = L'\0';

    return result;
}

void *memset(void *b, int c, size_t len)
{
    UINT8 *ptr = b;

    while (len--) {
        *ptr++ = (UINT8)c;
    }
    return b;
}