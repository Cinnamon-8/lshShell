
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/utsname.h>

char* lsh_read_line();
char** lsh_split_line(char* line);
void lsh_execute(char** tokens);
void lsh_exit(char **args);
void lsh_cd(char **args); 
void lsh_help(char **args);
void lsh_echo(char **args);
void lsh_pwd(char **args);
void lsh_mkdir(char **args);
void lsh_touch(char **args);
void lsh_cp(char **args);
void lsh_mv(char **args);
void lsh_find(char **args);
void lsh_history(char **args);
void lsh_top(char **args);
void lsh_clear(char **args);
void lsh_uname(char **args);
void lsh_symlink(char **args);
void lsh_rm(char **args);
void lsh_cat(char **args);

#define HISTORY_SIZE 10

char* history[HISTORY_SIZE];
int history_count = 0;


struct builtin {
    char *name;
    void (*func)(char **args);
};

struct builtin builtins[] = {
    {"help", lsh_help},
    {"exit", lsh_exit},
    {"cd", lsh_cd},
    {"echo", lsh_echo},
    {"pwd", lsh_pwd},
    {"mkdir", lsh_mkdir},
    {"touch", lsh_touch},
    {"cp", lsh_cp},
    {"mv", lsh_mv},
    {"find", lsh_find},
    {"history", lsh_history},
    {"top", lsh_top},
    {"clear", lsh_clear},
    {"uname", lsh_uname},
    {"symlink", lsh_symlink},
    {"rm", lsh_rm},
    {"cat", lsh_cat},
};

void lsh_uname(char **args) {
    struct utsname sys_info;
    if (uname(&sys_info) == -1) {
        perror("lsh: uname");
        return;
    }

    printf("System Name: %s\n", sys_info.sysname);
    printf("Node Name: %s\n", sys_info.nodename);
    printf("Release: %s\n", sys_info.release);
    printf("Version: %s\n", sys_info.version);
    printf("Machine: %s\n", sys_info.machine);
}

void lsh_rm(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: rm: missing argument\n");
        return;
    }

    if (remove(args[1]) == -1) {
        perror("lsh: rm");
        return;
    }

    printf("File removed: %s\n", args[1]);
}


void lsh_cat(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: cat: missing argument\n");
        return;
    }

    FILE *file = fopen(args[1], "r");
    if (file == NULL) {
        perror("lsh: cat");
        return;
    }

    int c;
    while ((c = fgetc(file)) != EOF) {
        putchar(c);
    }

    fclose(file);
}

void lsh_symlink(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "lsh: symlink: missing arguments\n");
        return;
    }

    if (symlink(args[1], args[2]) == -1) {
        perror("lsh: symlink");
        return;
    }

    printf("Symlink created: %s -> %s\n", args[2], args[1]);
}


void lsh_top(char **args) {
    // Open the process status file
    FILE *file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("lsh: top");
        return;
    }

    // Read and print the process information
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "procs_running", 13) == 0) {
            printf("Processes Running: %s", line + 13);
        } else if (strncmp(line, "procs_blocked", 13) == 0) {
            printf("Processes Blocked: %s", line + 13);
        }
    }

    fclose(file);
}

void lsh_touch(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: touch: missing argument\n");
        return;
    }

    FILE *file = fopen(args[1], "w");
    if (file == NULL) {
        perror("lsh: touch");
        return;
    }

    fclose(file);
}

void lsh_clear(char **args) {
    printf("\033[2J\033[H"); // ANSI escape sequence to clear screen and move cursor to top-left
}


void lsh_find(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: find: missing argument\n");
        return;
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("lsh: find");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, args[1]) == 0) {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
}

void lsh_mv(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "lsh: mv: missing arguments\n");
        return;
    }

    if (rename(args[1], args[2]) == -1) {
        perror("lsh: mv");
    }
}


void lsh_pwd(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("lsh: pwd");
    }
}

void lsh_cp(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "lsh: cp: missing arguments\n");
        return;
    }

    FILE *source = fopen(args[1], "r");
    if (source == NULL) {
        perror("lsh: cp");
        return;
    }

    FILE *destination = fopen(args[2], "w");
    if (destination == NULL) {
        fclose(source);
        perror("lsh: cp");
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytesRead, destination);
    }

    fclose(source);
    fclose(destination);
}


void lsh_mkdir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: mkdir: missing argument\n");
        return;
    }

    if (mkdir(args[1], 0777) == -1) {
        perror("lsh: mkdir");
    }
}


int lsh_num_builtins() {
    return sizeof(builtins) / sizeof(struct builtin);
}

void lsh_echo(char **args) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
}


char* lsh_read_line() {
    char *line = NULL;
    size_t buflen = 0;
    getline(&line, &buflen, stdin);
    return line;
}
char** lsh_split_line(char *line) {
    int length = 0;
    int capacity = 16;
    char **tokens = malloc(capacity * sizeof(char*));

    char *delimiters = " \t\r\n";
    char *token = strtok(line, delimiters);

    while (token != NULL) {
        tokens[length] = token;
        length++;

        if (length >= capacity) {
            capacity = (int) (capacity * 1.5);
            tokens = realloc(tokens, capacity * sizeof(char*));
        }

        token = strtok(NULL, delimiters);
    }

    tokens[length] = NULL;
    return tokens;
}

void lsh_execute(char **args) {

    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(args[0]);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(args[0]);
    }
    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args);
            return;
        }
    }

    pid_t child_pid = fork();

    if (child_pid == 0) {
        execvp(args[0], args);
        perror("lsh");
        exit(1);
    } else if (child_pid > 0) {
        int status;
        do {
            waitpid(child_pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    } else {
        perror("lsh");
    }
}
void lsh_exit(char **args) {
    exit(0);
}
void lsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh: cd");
        }
    }
}
void lsh_help(char **args) {
    printf("Welcome to Your Shell!\n");
    printf("The following commands are available:\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtins[i].name);
    }
}


void lsh_history(char **args) {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}



int main() {
    while (true) {
        printf("> ");
        char *line = lsh_read_line();
        char **tokens = lsh_split_line(line);

        if (tokens[0] != NULL) {
            lsh_execute(tokens);
        }

        free(tokens);
        free(line);
    }
}