#define _DEFAULT_SOURCE  // Это важно для lstat!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>
#include <sys/types.h>

#define MAX_PATH_LENGTH 1024

int is_text_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;
    return strcmp(dot, ".txt") == 0 || strcmp(dot, ".c") == 0 || 
           strcmp(dot, ".h") == 0 || strcmp(dot, ".cpp") == 0 ||
           strcmp(dot, ".md") == 0;
}

void search_in_file(const char *filename, const char *search_word, int ignore_case) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        return;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Ошибка получения информации о файле");
        close(fd);
        return;
    }

    if (sb.st_size == 0) {
        close(fd);
        return;
    }

    char *file_contents = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_contents == MAP_FAILED) {
        perror("Ошибка отображения файла");
        close(fd);
        return;
    }

    int line_number = 1;
    char *line_start = file_contents;
    char *current_pos = file_contents;

    while (current_pos < file_contents + sb.st_size) {
        if (*current_pos == '\n') {
            size_t line_length = current_pos - line_start;
            char line[line_length + 1];
            strncpy(line, line_start, line_length);
            line[line_length] = '\0';

            char *found_ptr;
            if (ignore_case) {
                char temp_line[line_length + 1];
                char temp_word[strlen(search_word) + 1];
                
                for (size_t i = 0; i < line_length; i++) {
                    temp_line[i] = tolower(line[i]);
                }
                temp_line[line_length] = '\0';
                
                for (size_t i = 0; i < strlen(search_word); i++) {
                    temp_word[i] = tolower(search_word[i]);
                }
                temp_word[strlen(search_word)] = '\0';
                
                found_ptr = strstr(temp_line, temp_word);
            } else {
                found_ptr = strstr(line, search_word);
            }

            if (found_ptr != NULL) {
                printf("%s:%d: %s\n", filename, line_number, line);
            }

            line_start = current_pos + 1;
            line_number++;
        }
        current_pos++;
    }

    munmap(file_contents, sb.st_size);
    close(fd);
}

void search_directory(const char *dirname, const char *search_word, int ignore_case) {
    DIR *dir = opendir(dirname);
    if (!dir) {
        perror("Ошибка открытия директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        struct stat statbuf;
        if (lstat(path, &statbuf) == -1) {
            perror("Ошибка получения информации о файле");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            search_directory(path, search_word, ignore_case);
        } else if (is_text_file(entry->d_name)) {
            search_in_file(path, search_word, ignore_case);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <директория> <слово> [-i]\n", argv[0]);
        fprintf(stderr, "Опции:\n");
        fprintf(stderr, "  -i  Игнорировать регистр\n");
        return 1;
    }

    const char *directory = argv[1];
    const char *search_word = argv[2];
    int ignore_case = 0;

    if (argc > 3 && strcmp(argv[3], "-i") == 0) {
        ignore_case = 1;
    }

    printf("Поиск слова '%s' в директории '%s'%s\n", 
           search_word, directory, 
           ignore_case ? " (игнорируя регистр)" : "");

    search_directory(directory, search_word, ignore_case);

    return 0;
}
