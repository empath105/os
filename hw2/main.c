#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

int main(int argc, char** argv) {
    const char* filename = "system_audit.txt";
    int ret = 0;

    char* username = getenv("USER");
    if (!username) username = getenv("USERNAME");
    if (!username) username = "UnknownUser";

    char* os_env = getenv("OS");
    if (!os_env) os_env = getenv("OSTYPE");
    if (!os_env) os_env = "UNIX-like system";

    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Не удалось создать файл отчета");
        return 1;
    }
    
    fprintf(f, "Пользователь: %s\n", username);
    fprintf(f, "Окружение ОС: %s\n", os_env);
    fclose(f);
    
    printf("Создан файл отчета '%s'\n", filename);

#ifdef _WIN64
    const char* utility = "findstr";
    const char* query = "Windows";
#else
    const char* utility = "grep";
    const char* query = "Windows";
#endif

    ret = run_and_wait(utility, query, filename);

    if (ret == 0) {
        printf("\nКлючевое слово найдено: это Windows\n");
    } else {
        printf("\nКлючевое слово не найдено: это Linux\n");
    }

    remove(filename);
    printf("Временный файл удален.\n");

    return 0;
}