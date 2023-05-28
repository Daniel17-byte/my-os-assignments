#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define RESP_PIPE "RESP_PIPE_85013"
#define REQ_PIPE "REQ_PIPE_85013"
#define SIZE 1024

int reqPipe = -1;
int respPipe = -1;

void* shm_ptr;
void* file_ptr;
struct stat sb;

void handle_write_shm() {
    unsigned int offset;
    unsigned int value;

    read(reqPipe, &offset, sizeof(unsigned int));
    read(reqPipe, &value, sizeof(unsigned int));


    if (offset > 4446308 - sizeof(unsigned int)) {
        write(respPipe, "WRITE_TO_SHM!", 13);
        write(respPipe, "ERROR!", 6);
        return;
    }

    *(unsigned int*)(shm_ptr + offset) = value;
    write(respPipe, "WRITE_TO_SHM!", 11);
    write(respPipe, "SUCCESS!", 8);
}

void handle_shm_create() {
    int shm = -1;
    int size;

    read(reqPipe, &size, sizeof(unsigned int));
    shm = shm_open("/v8LQFs", O_CREAT | O_RDWR, 0664);

    if (shm < 0) {
        write(respPipe, "CREATE_SHM!", 11);
        write(respPipe, "ERROR!", 6);
    } else {
        ftruncate(shm, size);
        char *mem = (char *) mmap(0, size * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
        if (mem == MAP_FAILED) {
            write(respPipe, "CREATE_SHM!", 11);
            write(respPipe, "ERROR!", 6);
        } else {
            write(respPipe, "CREATE_SHM!", 11);
            write(respPipe, "SUCCESS!", 8);
        }
    }
}

void handle_map_file() {
    char filename[256];
    char character;
    int size = 0;
    while (read(reqPipe, &character, 1) > 0){
        if(character == '!'){
            break;
        }
        filename[size++] = character;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        write(respPipe, "MAP_FILE!", 9);
        write(respPipe, "ERROR!", 6);
        return;
    }

    if (fstat(fd, &sb) == -1) {
        write(respPipe, "MAP_FILE!", 9);
        write(respPipe, "ERROR!", 6);
        return;
    }

    file_ptr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_ptr == MAP_FAILED) {
        write(respPipe, "MAP_FILE!", 9);
        write(respPipe, "ERROR!", 6);
        return;
    }

    write(respPipe, "MAP_FILE!", 9);
    write(respPipe, "SUCCESS!", 8);
}

void handle_read_from_file_offset() {
    unsigned int offset, no_of_bytes;

    read(reqPipe, &offset, sizeof(unsigned int));
    read(reqPipe, &no_of_bytes, sizeof(unsigned int));

    if (offset + no_of_bytes > sb.st_size) {
        write(respPipe, "READ_FROM_FILE_OFFSET!", 22);
        write(respPipe, "ERROR!", 6);
        return;
    }

    if (shm_ptr == NULL) {
        write(respPipe, "READ_FROM_FILE_OFFSET!", 22);
        write(respPipe, "ERROR!", 6);
        return;
    }

    memcpy(shm_ptr, file_ptr + offset, no_of_bytes);

    write(respPipe, "READ_FROM_FILE_OFFSET!", 22);
    write(respPipe, "SUCCESS!", 8);
}

void handle_read_from_file_section() {
    unsigned int section_no, offset, no_of_bytes;
    unsigned int section_offset = 0, section_size = 0;

    read(reqPipe, &section_no, sizeof(unsigned int));
    read(reqPipe, &offset, sizeof(unsigned int));
    read(reqPipe, &no_of_bytes, sizeof(unsigned int));

    if (offset + no_of_bytes > section_size) {
        write(respPipe, "READ_FROM_FILE_SECTION!",23);
        write(respPipe,"ERROR!",6);
        return;
    }

    if (shm_ptr == NULL || file_ptr == NULL) {
        write(respPipe, "READ_FROM_FILE_SECTION!",23);
        write(respPipe,"ERROR!",6);
        return;
    }

    memcpy(shm_ptr, file_ptr + section_offset + offset, no_of_bytes);

    write(respPipe, "READ_FROM_FILE_SECTION!",23);
    write(respPipe,"SUCCESS!",8);
}

void handle_read_from_logical_space_offset() {
    unsigned int logical_offset, no_of_bytes;
    unsigned int logical_mem_start = 0;
    unsigned int file_size = 0;

    read(reqPipe, &logical_offset, sizeof(unsigned int));
    read(reqPipe, &no_of_bytes, sizeof(unsigned int));

    unsigned int file_offset = logical_mem_start + logical_offset;

    if (file_offset + no_of_bytes > file_size) {
        write(respPipe, "READ_FROM_LOGICAL_SPACE_OFFSET!", 31);
        write(respPipe, "ERROR!", 6);
        return;
    }

    if (shm_ptr == NULL) {
        write(respPipe, "READ_FROM_LOGICAL_SPACE_OFFSET!", 31);
        write(respPipe, "ERROR!", 6);
        return;
    }

    memcpy(shm_ptr, file_ptr + file_offset, no_of_bytes);

    write(respPipe, "READ_FROM_LOGICAL_SPACE_OFFSET!", 31);
    write(respPipe, "SUCCESS!", 8);
}

int main() {
    char buffer[SIZE] = "\n";

    if(mkfifo(RESP_PIPE, 0600) != 0){
        perror("ERROR\ncannot create the response pipe\n");
        return 1;
    }

    reqPipe = open(REQ_PIPE, O_RDONLY);
    if(reqPipe == -1){
        perror("ERROR\ncannot open the request pipe\n");
        unlink(RESP_PIPE);
        return 1;
    }

    respPipe = open(RESP_PIPE, O_WRONLY);
    if(respPipe == -1) {
        perror("ERROR\ncannot open the response pipe\n");
        unlink(RESP_PIPE);
        close(reqPipe);
        unlink(RESP_PIPE);
        return 1;
    }

    write(respPipe, "CONNECT!", 8);

    printf("SUCCESS");

    while (1) {
        memset(buffer, 0, SIZE);
        int size = 0;
        char character;
        int bool = 0;
        while (read(reqPipe, &character, 1) > 0){
            if(character == '!'){
                bool = 1;
            }
            buffer[size++] = character;
            if(bool){
                break;
            }
        }

        if (strcmp(buffer, "ECHO!") == 0) {
            unsigned int variant = 85013;
            write(respPipe, "ECHO!", 5);
            write(respPipe, "VARIANT!", 8);
            write(respPipe, &variant, sizeof(unsigned int));
            break;
        }

        if (strcmp(buffer, "CREATE_SHM!") == 0) {
            handle_shm_create();
        }
        if (strcmp(buffer, "WRITE_TO_SHM!") == 0) {
            handle_write_shm();
        }

        if (strcmp(buffer, "MAP_FILE!") == 0) {
            handle_map_file();
        }

        if (strcmp(buffer, "READ_FROM_FILE_OFFSET!") == 0) {
            handle_read_from_file_offset();
        }

        if (strcmp(buffer, "READ_FROM_FILE_SECTION!") == 0) {
            handle_read_from_file_section();
        }

        if (strcmp(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET!") == 0) {
            handle_read_from_logical_space_offset();
        }

        if (strcmp(buffer, "EXIT!") == 0) {
            write(respPipe,"EXIT!",5);
            close(reqPipe);
            close(respPipe);
            unlink(RESP_PIPE);
            return 0;
        }
    }
    return 0;
}