#include <msh.h>
#include <msh_parse.h>
#include "mshparse/msh_parse.c"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h> //for the wronly to work
#include <sys/wait.h>

pid_t foreground_pid;
struct msh_pipeline* foreground_pipe = NULL;
struct msh_pipeline *array[16];
int bg_index = 0; 

 	struct proc_data {
	    pid_t pid_c;
	};

void sig_handler(int signal_number, siginfo_t *nfo, void *ctxt) {
    (void) *nfo;
    (void) ctxt;
    switch (signal_number) {
        case SIGTERM:
            if (foreground_pid > 0) {
                kill(foreground_pid, SIGTERM); 
                foreground_pid = 0; 
            }
            break;

        case SIGTSTP: // control-z
            if (foreground_pid > 0) {
                kill(foreground_pid, SIGTSTP); 

                int stored = 0;
                for (int i = 0; i < 16; i++) {
                    if (array[i] == NULL) {
                        array[i] = foreground_pipe; // Store the current foreground pipeline
                        stored = 1;
                        break;
                    }
                }

                if (!stored) {
                    fprintf(stderr, "Error: No space left to store suspended jobs.\n");
                }

                foreground_pid = 0; // Reset the foreground_pid
                foreground_pipe = NULL; // Reset foreground_pipe
            }
            break;
    }
}

void msh_execute(struct msh_pipeline *p) {
    int fds[2];
    int next;
    int status;
    struct msh_command *command;
    int num_commands = 0;

    while (msh_pipeline_command(p, num_commands) != NULL) {
        num_commands++;
    }

    if (num_commands > 1) {
        for (int i = 0; i < num_commands; i++) {
            command = msh_pipeline_command(p, i);
            char **args = msh_command_args(command);

            if (i != num_commands - 1) {
                pipe(fds);
            }

            pid_t pid = fork();

            if (pid == 0) { // Child process
                if (i != 0) {
                    close(STDIN_FILENO);
                    dup2(next, STDIN_FILENO);
                }
                if (i != num_commands - 1) {
                    close(STDOUT_FILENO);
                    dup2(fds[1], STDOUT_FILENO);
                    close(fds[1]);
                }
                // do a for loop through 0 to 16, then find the file re direct then you know that after the file re direct is the file. 
                // Handle stdout redirection
                // 0700 given
                if (command->stdout_file) {
                    int flags = O_WRONLY | O_CREAT | (command->stdout_append ? O_APPEND : O_TRUNC);
                    int fd_out = open(command->stdout_file, flags, 0700);
                    if (fd_out == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                // Handle stderr redirection
                if (command->stderr_file) {
                    int fd_err = open(command->stderr_file, O_WRONLY | O_CREAT | O_TRUNC, 0700);
                    if (fd_err == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_err, STDERR_FILENO);
                    close(fd_err);
                }

                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else if (pid > 0) { // Parent process
                if (i != 0) {
                    close(next);
                }
                if (i != num_commands - 1) {
                    close(fds[1]);
                    next = fds[0];
                }
                if (i == num_commands - 1) { // Last command in pipeline
                    foreground_pid = pid; 
                    foreground_pipe = p;  // Set foreground_pipe
                    waitpid(pid, &status, 0); 
                    foreground_pid = 0; 
                }
            } else {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        command = msh_pipeline_command(p, 0);
        char **args = msh_command_args(command);

        if (strcmp(args[0], "cd") == 0) {
            char *dir = args[1] ? args[1] : getenv("HOME");
            if (chdir(dir) != 0) {
                perror("cd");
            }
        } else if (strcmp(args[0], "exit") == 0) {
            exit(0);
        } else {
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) { // Child process
                // Handle standard output redirection
                if (command->stdout_file) {
                    int flags = O_WRONLY | O_CREAT | (command->stdout_append ? O_APPEND : O_TRUNC);

                    int fd_out = open(command->stdout_file, flags, 0700);
                    if (fd_out == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                // Handle standard error redirection
                if (command->stderr_file) {
                    int fd_err = open(command->stderr_file, O_WRONLY | O_CREAT | O_TRUNC, 0700);
                    if (fd_err == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd_err, STDERR_FILENO);
                    close(fd_err);
                }

                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else { 
                foreground_pid = pid; 
                foreground_pipe = p;  
                waitpid(pid, &status, 0); 
                foreground_pid = 0; 
            }
        }
    }
                msh_pipeline_free(p);
}
   
void setup_signal(int signo, void (*fn)(int signo, siginfo_t *nfo, void *ctxt)) {
    struct sigaction siginfo;

    sigemptyset(&siginfo.sa_mask);
    siginfo.sa_sigaction = fn;
    siginfo.sa_flags = SA_SIGINFO;

    if (sigaction(signo, &siginfo, NULL) == -1) {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }
}



void msh_init(void) {
    setup_signal(SIGINT, sig_handler); 
    setup_signal(SIGTSTP, sig_handler);
}