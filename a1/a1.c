#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

int stat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);

void variant() {
    printf("%s\n", "85013");
}

void list_directory(const char *path, int recursive, const char *name_ends_with, off_t size_greater) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat st;
    char file_path[1024];

    if (!dir) {
        printf("ERROR\ninvalid directory path\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
        stat(file_path, &st);

        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            if (recursive) {
                list_directory(file_path, recursive, name_ends_with, size_greater);
            }

            if (name_ends_with && strstr(entry->d_name, name_ends_with) == NULL) {
                continue;
            }

            printf("%s\n", file_path);
        } else {
            if (size_greater >= 0 && st.st_size <= size_greater) {
                continue;
            }

            if (name_ends_with && strstr(entry->d_name, name_ends_with) == NULL) {
                continue;
            }

            printf("%s\n", file_path);
        }
    }

    closedir(dir);
}

void list(int argc, char **argv) {
    int recursive = 0;
    off_t size_greater = -1;
    char *name_ends_with = NULL;
    char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "recursive") == 0) {
            recursive = 1;
        } else if (strncmp(argv[i], "size_greater=", 13) == 0) {
            size_greater = atoll(argv[i] + 13);
        } else if (strncmp(argv[i], "name_ends_with=", 15) == 0) {
            name_ends_with = argv[i] + 15;
        } else if (strncmp(argv[i], "path=", 5) == 0) {
            path = argv[i] + 5;
        }
    }

    if (path == NULL) {
        printf("ERROR\nmissing directory path\n");
        return;
    }

    printf("SUCCESS\n");
    list_directory(path, recursive, name_ends_with, size_greater);
}

void parse(int argc, char **argv) {
    char *path = NULL;
    struct stat file_stat;
    char *MAGIC = "5p";
    int MIN_VERSION = 122;
    int MAX_VERSION = 161;
    int MIN_SECTIONS = 3;
    int MAX_SECTIONS = 11;
    int VALID_SECTION_TYPES[] = {99, 96, 10, 75, 73, 39};
    int num_valid_types = sizeof(VALID_SECTION_TYPES) / sizeof(VALID_SECTION_TYPES[0]);
    char magic[2];
    int version;
    int nr_sections;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "path=", 5) == 0) {
            path = argv[i] + 5;
            break;
        }
    }

    if (path == NULL) {
        printf("ERROR\nmissing file path\n");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nopen\n");
        return;
    }

    if (fstat(fd, &file_stat) == -1) {
        printf("ERROR\nfstat");
        close(fd);
        return;
    }

    if (file_stat.st_size < sizeof(int) * 3) {
        printf("ERROR\nfile size too small\n");
        close(fd);
        return;
    }

    off_t headersize;
    lseek(fd, 0, SEEK_END);
    lseek(fd, -4, SEEK_CUR);
    read(fd, &headersize, 2);

    read(fd, magic, 2);
    if (strncmp(magic, MAGIC, 2) != 0) {
        printf( "ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    lseek(fd, -headersize, SEEK_CUR);

    read(fd, &version, 2);
    if (version < MIN_VERSION || version > MAX_VERSION) {
        printf("ERROR\nwrong version\n");
        close(fd);
        return;
    }

    read(fd, &nr_sections, 1);
    if (nr_sections < MIN_SECTIONS || nr_sections > MAX_SECTIONS) {
        printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return;
    }

    int valid = 1;
    for (int i = 0; i < nr_sections; i++) {
        char name[10];
        int type, size;
        read(fd, name, 10);
        read(fd, &type, 4);
        read(fd, &size, 4);

        int valid_type = 0;
        for (int j = 0; j < num_valid_types; j++) {
            if (type == VALID_SECTION_TYPES[j]) {
                valid_type = 1;
                break;
            }
        }

        if (valid_type == 0) {
            valid = 0;
            break;
        }
    }

    if (valid == 0) {
        printf("ERROR\nwrong sect_types\n");
    } else {
        printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, nr_sections);
        lseek(fd, - (18 * nr_sections), SEEK_CUR);
        for (int i = 0; i < nr_sections; i++) {
            char name[10];
            int type, size;
            read(fd, name, 10);
            name[10]='\0';
            read(fd, &type, 4);
            read(fd, &size, 4);

            printf("section%d: %s %d %d\n", i + 1, name, type, size);
        }
    }

    close(fd);
    return;
}

void extract(int argc, char **argv) {
    char *path = (char *)malloc(256 * sizeof(char));
    int target_section = -1, target_line = -1;
    int i;

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "path=", 5) == 0) {
            strcpy(path, argv[i] + 5);
        } else if (strncmp(argv[i], "section=", 8) == 0) {
            target_section = atoi(argv[i] + 8);
        } else if (strncmp(argv[i], "line=", 5) == 0) {
            target_line = atoi(argv[i] + 5);
        }
    }

    if (target_section == -1 || target_line == -1) {
        printf("ERROR\ninvalid argument\n");
        free(path);
        return;
    }

    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        printf("ERROR\ninvalid file\n");
        free(path);
        return;
    }

    char magic[2];
    read(fd, magic, 2);
    magic[2] = '\0';

    int version, nr_sections;
    read(fd, &version, 2);
    read(fd, &nr_sections, 1);

    if (target_section > nr_sections) {
        printf("ERROR\ninvalid section\n");
        close(fd);
        free(path);
        return;
    }

    for (i = 1; i < target_section; i++) {
        char section_name[10];
        int section_type, section_size;

        read(fd, section_name, 10);
        section_name[10] = '\0';
        read(fd, &section_type, 4);
        read(fd, &section_size, 4);

        lseek(fd, section_size, SEEK_CUR);
    }

    char section_name[10];
    int section_type, section_size;
    read(fd, section_name, 10);
    section_name[10] = '\0';
    read(fd, &section_type, 4);
    read(fd, &section_size, 4);

    int current_line = 1;
    char ch;
    char line[256];
    int line_index = 0;

    for (i = 0; i < section_size; i++) {
        read(fd, &ch, 1);

        if (ch == '\n') {
            if (current_line == target_line) {
                line[line_index] = '\0';
                break;
            }
            current_line++;
            line_index = 0;
        } else {
            line[line_index++] = ch;
        }
    }

    if (current_line != target_line) {
        printf("ERROR\ninvalid line\n");
        close(fd);
        free(path);
        return;
    }

    printf("SUCCESS\n");
    for (i = strlen(line) - 1; i >= 0; i--) {
        printf("%c", line[i]);
    }
    printf("\n");

    close(fd);
    free(path);
}

int is_valid_sf_file(char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 0;
    }

    char magic[] = "5p";
    char magic_read[2];

    off_t headersize;
    lseek(fd, 0, SEEK_END);
    lseek(fd, -4, SEEK_CUR);
    read(fd, &headersize, 2);

    read(fd, magic_read, 2);
    magic_read[2] = '\0';
    if (strcmp(magic_read, magic) != 0) {
        close(fd);
        return 0;
    }

    lseek(fd, -headersize, SEEK_CUR);

    int version, nr_sections;
    read(fd, &version, 2);
    read(fd, &nr_sections, 1);

    for (int i = 0; i < nr_sections; i++) {
        char section_name[10];
        int section_type, section_size;

        read(fd, section_name, 10);
        read(fd, &section_type, 4);
        read(fd, &section_size, 4);

        if (section_size > 950) {
            close(fd);
            return 0;
        }
    }

    close(fd);
    return 1;
}

void find_sf_files(char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char subdir_path[1024];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", path, entry->d_name);
            find_sf_files(subdir_path);
        } else {
            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
            if (is_valid_sf_file(file_path)) {
                printf("%s\n", file_path);
            }
        }
    }

    closedir(dir);
}

void findall(int argc, char **argv) {
    char *path = NULL;
    int i;

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "path=", 5) == 0) {
            path = argv[i] + 5;
        }
    }

    DIR *dir = opendir(path);
    if (!dir) {
        printf("ERROR\ninvalid directory path\n");
        return;
    }
    closedir(dir);

    printf("SUCCESS\n");
    find_sf_files(path);
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        if (strcmp(argv[1], "variant") == 0) {
            variant();
        } else if (strcmp(argv[1], "list") == 0) {
            list(argc, argv);
        } else if (strcmp(argv[1], "parse") == 0) {
            parse(argc, argv);
        } else if (strcmp(argv[1], "extract") == 0) {
            extract(argc, argv);
        } else if (strcmp(argv[1], "findall") == 0) {
            findall(argc, argv);
        }
    }
    return 0;
}