#include <Uefi.h>
#include <Base.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>

#include "boot_config.h"
#include "utils.h"

typedef enum {
    KEY,
    VALUE,
    MV_UNTIL_NEXT_KEY
} ReadingState;

CHAR8 *read_config_property(
    CONST CHAR8 *key,
    CONST CHAR8 *file_buff,
    CONST UINT64 file_buff_size,
    CONST EFI_BOOT_SERVICES *gBS
) {
    Puts(L"read_config_property"); /* Temp debug */

    ReadingState read_st       = KEY;
    short k_buff_i             = 0,
          v_buff_i             = 0;
    CHAR8 k_buff[CONFIG_LINE_SIZE+1];
    CHAR8 *v_buff;

    if (!*file_buff) { Puts(L"E"); }

    EFI_STATUS alloc_stat = gBS->AllocatePool(
        EfiLoaderData,
        CONFIG_LINE_SIZE+1,
        (VOID**) &v_buff
    );
    if (EFI_ERROR(alloc_stat)) { return NULL; }

    Print(L"File buffer size - %llu\n", file_buff_size);
    for (CONST CHAR8 *read_pos = file_buff; read_pos - file_buff < file_buff_size; read_pos++) {

        switch (read_st) {

            case MV_UNTIL_NEXT_KEY:
                Puts(L"MV_UNTIL_NEXT_KEY");
                if (*read_pos == '\n') {
                    read_st = KEY;
                } else {
                    read_pos++;
                }
                break;

            case KEY:
                Puts(L"KEY");
                switch (*read_pos) {

                    case '#':
                        read_st = MV_UNTIL_NEXT_KEY;
                        break;
                    
                    case '=':
                        k_buff[k_buff_i] = '\0';
                        if (AsciiStrCmp(key, k_buff) == 0) {
                            read_st = VALUE;
                            break;
                        }
                        k_buff_i = 0;
                        read_st  = MV_UNTIL_NEXT_KEY;
                        break;
                    
                    case ' ':
                        break;

                    default:
                        k_buff[k_buff_i++] = *read_pos;
                }
                break;

            case VALUE:
                Puts(L"VALUE");
                if (
                    *read_pos == '#' ||
                    *read_pos == '\r' ||
                    *read_pos == '\n'
                ) {
                    v_buff[v_buff_i] = '\0';
                    return v_buff;
                }
                if (*read_pos != ' ') { v_buff[v_buff_i++] = *read_pos; }
                break;
        }
    }
    return NULL;
}
