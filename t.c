#include <stdlib.h>

typedef struct {
    char x;
    char y;
} EFI_FILE_INFO;

void som(
    EFI_FILE_INFO **info
) {
    *info = (EFI_FILE_INFO*) malloc(sizeof(EFI_FILE_INFO));
}

void x(
    EFI_FILE_INFO **info
) {
    ...
    stat = sub->GetInfo(
        sub,
        &gEfiFileInfoGuid,
        &buff_size,
        (VOID**) info
    );
    ...
}

int main() {
    EFI_FILE_INFO *info;
    x(&info);
    gBS->FreePool(info);
    return 0;
}
