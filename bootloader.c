#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#define KERNEL_FILE_PRE "vm_linuz"
#define BOOT_DIR "/boot"
#define BOOT_CONFIG_FILE_LOCATIONS {"/proc/bootconfig", "/boot/config"}

bool permitted_kernel_file(struct dirent *entry) {

    return (
        access(entry->d_name, R_OK) == 0 &&
        strncmp(KERNEL_FILE_PRE, entry->d_name, strlen(KERNEL_FILE_PRE)) == 0
    );
}

char * find_in_boot_dir() {

    DIR *dir;
    if ((dir = opendir(BOOT_DIR)) == NULL) {
        return NULL;
    }

    struct dirent *direntry;

    while ((direntry = readdir(dir)) != NULL) {

        switch (direntry->d_type) {

            case DT_REG:
                if (permitted_kernel_file(direntry)) {
                    return direntry->d_name;
                }
                break;

            case DT_UNKNOWN:
                struct stat s_stat;

                if (stat(direntry->d_name, &s_stat) == 0) {
                    if (permitted_kernel_file(direntry)) {
                        return direntry->d_name;
                    }
                }
        }
    }
    return NULL;
}

typedef enum {
    COMMENT,
    KEY,
    VALUE
} ReadingState;

#define CONFIG_LINE_SIZE (256)

char mv_to_line_end(FILE *file) {
    char c;

    while (c = fgetc(file) != '\n' && c != EOF) { }
    return c;
}

char * get_config_property(char *key, char *path) {
    
    FILE *file;
    char *source;
    struct stat st;


    // Open file
    if (
        lstat(path, &st) != 0 ||
        (S_IFREG != (st.st_mode & S_IFMT)) ||
        (file = fopen(path, "rb")) == NULL
    ) {
        return NULL;
    }

    // +1 offset for \0
    char  k_buff[CONFIG_LINE_SIZE+1], v_buff[CONFIG_LINE_SIZE+1];
    short k_buff_i = 0,               v_buff_i = 0;
    ReadingState r_st = KEY;
    char c;

    // Reading Config
    switch (r_st) {

        case COMMENT:
            if (mv_to_line_end == EOF) return NULL;

            r_st = KEY;
            break;

        case KEY:
            while (
                (c = fgetc(file)) != '#' &&
                c != '=' &&
                c != EOF
            ) {
                if (c != ' ') k_buff[k_buff_i++] = c;
            }
            switch (c) {
                case '#':
                    r_st = COMMENT;
                    break;
                
                case '=':
                    k_buff[k_buff_i] = '\0';
                    if (strcmp(key, k_buff)) {
                        r_st = VALUE;
                        break;
                    }
                    if (mv_to_line_end == EOF) return NULL;
                    // Overwriting char that are in the buffer
                    k_buff_i = 0;
                    break;
                
                case EOF:
                    return NULL;
            }
            break;

            case VALUE:
                while (
                    (c = fgetc(file)) != '#' &&
                    c != '\n' &&
                    c != EOF
                ) {
                    if (c != ' ') v_buff[v_buff_i++] = c;
                }
                v_buff[v_buff_i] = '\0';
                return v_buff;
    }
}


int main() {

    // Kernel file location
    char *kernel_file_path = NULL;

    // Checking for boot config file
    const int BOOT_CONFIG_FILES[] = BOOT_CONFIG_FILE_LOCATIONS;

    for (
        size_t *cur_conf_file = BOOT_CONFIG_FILES;
        cur_conf_file < sizeof(BOOT_CONFIG_FILES) / sizeof(BOOT_CONFIG_FILES[0]);
        cur_conf_file++
    ) {
        kernel_file_path = get_config_property("kernel", *cur_conf_file);
    }

    // Fallback if not found
    if (kernel_file_path != NULL) kernel_file_path = find_in_boot_dir();

    if (kernel_file_path == NULL) return 1;
}
