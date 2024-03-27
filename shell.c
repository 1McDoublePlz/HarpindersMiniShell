#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "helpers.h"

#define MAX_COMMAND_LENGTH 1024
#define MAX_BACKGROUND_PROCESSES 10
#define PATH_MAX 4096

//definition for background processes
typedef struct {
    pid_t pid;
    char command[MAX_COMMAND_LENGTH];
} BackgroundProcess;

BackgroundProcess background_processes[MAX_BACKGROUND_PROCESSES] = {0};

//adding a background process
int add_background_process(pid_t pid, char *command) {
        //loops until an empty spot is found
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
        if (background_processes[i].pid == 0) {
            background_processes[i].pid = pid;
            strncpy(background_processes[i].command, command, MAX_COMMAND_LENGTH);
            return 0;// successfully added a process
        }
    }
    return -1;// no space left for a new process
}

//wait for all background processes to finish
void wait_for_background_processes() {
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
        if (background_processes[i].pid != 0) {
            waitpid(background_processes[i].pid, NULL, 0);//waiting for specific child process
            printf("Background process %d with PID %d finished: %s\n", i + 1, background_processes[i].pid, background_processes[i].command);
            background_processes[i].pid = 0;//reset the slot
        }
    }
}


//built in help
void printHelp() {
    printf("\nBuilt-in commands:\n");
    printf("help - Display this help message\n");
    printf("exit - Exit the shell\n");
    printf("pwd - Print the current working directory\n");
    printf("cd <path> - Change the current working directory to <path>\n");
    printf("wait - Wait for all background processes to finish\n");
    printf("& - Run command in background (e.g., /bin/ls &)\n\n");
}


//built in pwd command
void printPwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}


//built-in change direcotry command
void runCd(char *path) {
    if (chdir(path) != 0) {
        perror("cd");
    }
}

//execution of programs with and without explicit paths
void execProg(char **args) {
    int status;
    pid_t pid;
    int num_pipes = 0;
    int inFd = -1, outFd = -1; //file descriptors for input and output redirection

    //count the number of pipes and prepare for redirection
    for (int i = 0; args[i] != NULL; i++) {
            //for pipes
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL; //null-terminate at pipes for command segmentation
            num_pipes++;
        } else if (strcmp(args[i], ">") == 0 && args[i + 1] != NULL) {
                //for output redirection
            outFd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outFd == -1) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL; //remove the redirection symbol and filename from the arguments
            i++; //skip over the filename

            //for input redirection
        } else if (strcmp(args[i], "<") == 0 && args[i + 1] != NULL) {
            inFd = open(args[i + 1], O_RDONLY);
            if (inFd == -1) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL; //remove the redirection symbol and filename from the arguments
            i++; //skip over the filename
        }
    }

    //create an array for pipes
    int pipe_arr[num_pipes][2];
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_arr[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    int currentArg = 0; //index to track the start of the current command segment

    for (int i = 0; i <= num_pipes; i++) {
        pid = fork();
        if (pid == 0) { //dhild process
                        //setting up stdin/stdout redirection ofr pipes
                        //"correct use of exec "

            if (i > 0) {
                dup2(pipe_arr[i - 1][0], STDIN_FILENO); //redirect stdin for pipes
                close(pipe_arr[i - 1][1]);
            } else if (inFd != -1) {
                dup2(inFd, STDIN_FILENO); //redirect stdin from file
                close(inFd);
            }

            if (i < num_pipes) {
                dup2(pipe_arr[i][1], STDOUT_FILENO); //redirect stdout for pipes
                close(pipe_arr[i][0]);
            } else if (outFd != -1) {
                dup2(outFd, STDOUT_FILENO); //redirect stdout to file
                close(outFd);
            }

            //close all other pipes' file descriptors
            for (int j = 0; j < num_pipes; j++) {
                if (j != i) close(pipe_arr[j][0]);
                if (j != i - 1) close(pipe_arr[j][1]);
            }

            execvp(args[currentArg], &args[currentArg]);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { //parent process
                 // closing pipe ends appropriately in parent
            if (i > 0) {
                close(pipe_arr[i - 1][0]); //close read end of the previous pipe
            }
            if (i < num_pipes) {
                close(pipe_arr[i][1]); //close write end of the current pipe
            }
        }

        //find the start of the next command segment
        while(args[currentArg] != NULL) currentArg++;
        currentArg++; // Skip the NULL to find the start of the next command
    }

    //close any remaining open file descriptors
    if (inFd != -1) {
        close(inFd);
    }
    if (outFd != -1) {
        close(outFd);
    }

    //wait for child processes to finish
    for (int i = 0; i <= num_pipes; i++) {
        wait(&status);
    }
}


int main() {
    char commandLine[MAX_COMMAND_LENGTH];
    printf("\nHello! Welcome to your Mini-Shell.\nPlease enter your command: ");

    while (1) {
        printf("\nMini-Shell> ");

        //reads input from user
        if (!fgets(commandLine, MAX_COMMAND_LENGTH, stdin)) {
            printf("Failed to read input\n");
            continue;
        }

        commandLine[strcspn(commandLine, "\n")] = 0; //remove newline character

        //built in commands
        if (strcmp(commandLine, "exit") == 0) {
            break;
        } else if (strcmp(commandLine, "help") == 0) {
            printHelp();
        } else if (strcmp(commandLine, "pwd") == 0) {
            printPwd();
        } else if (strncmp(commandLine, "cd ", 3) == 0) {
            runCd(commandLine + 3);
        } else if (strcmp(commandLine, "wait") == 0) {
            wait_for_background_processes();
        } else {
                //if its not built in parse the input
            char **args = parse(commandLine, " ");
            if (args == NULL) {
                continue; //error handling or empty command
            }

            //checking for background execution request
            int isBackground = 0;
            for (int i = 0; args[i] != NULL; i++) {
                if (strcmp(args[i], "&") == 0) {
                    isBackground = 1;
                    args[i] = NULL; //remove the & for execution
                    break;
                }
            }

            //runing parsed command in the background if needed
            if (isBackground) {
                pid_t pid = fork();
                if (pid == 0) { //child
                    execProg(args);
                    exit(EXIT_SUCCESS);
                } else if (pid > 0) { //parent
                    add_background_process(pid, commandLine);
                    printf("Started background job %d: %s\n", pid, commandLine);
                }
            } else {
                execProg(args); //handle foreground execution
            }

            free(args); //free parsed arguments
        }
    }
    return 0;
}

~                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
~                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
~                         
