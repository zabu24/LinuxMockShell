#include <msh_parse.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <ctype.h>
#define MSH_ERR_MEM_ALLOC -1 


struct msh_command {
	char *args[MSH_MAXARGS + 2];
	size_t args_count;
	int last_cmd;
    char *stdout_file; // File -> standard output (new)
    char *stderr_file; // File -> redirecting standard error (new)
    int stdout_append; // Flag -> appending for stdout (new)
    int stderr_append; // Flag -> appending for stderr (new)

	void *pid;
	msh_free_data_fn_t freefn;
    
};

struct msh_pipeline {
	struct msh_command *commands[MSH_MAXCMNDS];
	size_t cmd_count;
	char* parsed_pl;
	int background; 
	unsigned int cmd_index;
};

struct msh_sequence {
	struct msh_pipeline *pipelines[MSH_MAXBACKGROUND + 1];
	size_t pl_count;
	size_t pl_index;
};

void msh_command_free(struct msh_command *c) {
    
    for (size_t i = 0; i < MSH_MAXARGS; i++) {

        free(c->args[i]);
    }

}
void msh_pipeline_free(struct msh_pipeline *p)
{
    if (p == NULL) {
        return;
    }

    for (size_t i = 0; i < p->cmd_count; i++) {
        struct msh_command *cmd = p->commands[i];
        
        if (cmd == NULL) {
            continue;
        }

        for (size_t j = 0; j < cmd->args_count; j++) {
            free(cmd->args[j]);
        }

        free(cmd);
    }

    free(p->parsed_pl);

    free(p);
}

void
msh_sequence_free(struct msh_sequence *s)
{
	for (size_t i = 0; i < s -> pl_count; i++) {
		msh_pipeline_free(s -> pipelines[i]);
	}
	free(s);
}

struct msh_sequence *
msh_sequence_alloc(void)
{
	struct msh_sequence *seq = (struct msh_sequence *)calloc(1, sizeof(struct msh_sequence));
    if (seq == NULL) {
		return NULL;
    }
	for (size_t i = 0; i <= MSH_MAXBACKGROUND; i++) {
		seq -> pipelines[i] = NULL;
	}
	seq -> pl_count = 0;
	seq -> pl_index = 0;
	return seq;
}

char *
msh_pipeline_input(struct msh_pipeline *p)
{
	return p -> parsed_pl;
}
msh_err_t msh_sequence_parse(char *str, struct msh_sequence *seq)
{
    if (seq == NULL || str == NULL || strlen(str) == 0) {
        return -1; 
    }
    char *input_copy = strdup(str);
    char *command_str;
    int loop1 = 0;
    int loop2 = 0;
    int loop3 = 0;

    for (char *token = strtok_r(input_copy, ";", &command_str); token != NULL; token = strtok_r(NULL, ";", &command_str)) {
        seq->pipelines[loop1] = (struct msh_pipeline *)calloc(1, sizeof(struct msh_pipeline));
        if (seq->pipelines[loop1] == NULL) {
            free(input_copy);
            return MSH_ERR_MEM_ALLOC;
        }

        seq->pipelines[loop1]->parsed_pl = strdup(token);
        char *token2Rest = token;
        char *pipeline_str;

        for (char *token2 = strtok_r(token2Rest, "|", &pipeline_str); token2 != NULL; token2 = strtok_r(NULL, "|", &pipeline_str)) {
            if (loop2 >= MSH_MAXCMNDS) {
                free(token2Rest);
                return MSH_ERR_TOO_MANY_CMDS;
            }

            int cmnd_empty = 1;
            for (char *pointer = token2; *pointer != '\0'; pointer++) {
                if (!isspace(*pointer)) {
                    cmnd_empty = 0;
                    break;
                }
            }

            if (cmnd_empty) {
                free(token2Rest);
                return MSH_ERR_PIPE_MISSING_CMD;
            }

            seq->pipelines[loop1]->commands[loop2] = (struct msh_command *)calloc(1, sizeof(struct msh_command));
            if (seq->pipelines[loop1]->commands[loop2] == NULL) {
                free(input_copy);
                return MSH_ERR_MEM_ALLOC;
            }

            char *token3Rest = token2;
            char *arg_str;
            loop3 = 0;

            for (char *token3 = strtok_r(token3Rest, " ", &arg_str); token3 != NULL; token3 = strtok_r(NULL, " ", &arg_str)) {
                if (loop3 >= MSH_MAXARGS) {
                    free(input_copy);
                    return MSH_ERR_TOO_MANY_ARGS;
                }

                if (strcmp(token3, "1>") == 0 || strcmp(token3, "2>") == 0 || strcmp(token3, "1>>") == 0) {
                    char *redir_type = strdup(token3);
                    token3 = strtok_r(NULL, " ", &arg_str); // Get the next token, which should be the filename

                    if (!token3) {
                        free(redir_type);
                        free(input_copy);
                        return MSH_ERR_MULT_REDIRECTIONS;
                    } else {
                        if (strcmp(redir_type, "1>") == 0) {
                            seq->pipelines[loop1]->commands[loop2]->stdout_file = strdup(token3);
                            seq->pipelines[loop1]->commands[loop2]->stdout_append = 0;
                        } else if (strcmp(redir_type, "2>") == 0) {
                            seq->pipelines[loop1]->commands[loop2]->stderr_file = strdup(token3);
                        } else if (strcmp(redir_type, "1>>") == 0) {
                            seq->pipelines[loop1]->commands[loop2]->stdout_file = strdup(token3);
                            seq->pipelines[loop1]->commands[loop2]->stdout_append = 1;
                        }
                        free(redir_type);
                        continue; 
                    }
                } else {
                    seq->pipelines[loop1]->commands[loop2]->args[loop3] = strdup(token3);
                    loop3++;
                }
            }

            seq->pipelines[loop1]->commands[loop2]->args_count = loop3;
            loop2++;
        }

        seq->pipelines[loop1]->cmd_count = loop2;
        if (loop2 > 0) {
            seq->pipelines[loop1]->commands[loop2 - 1]->last_cmd = 1;
        }

        loop1++;
        loop2 = 0;
        seq->pl_count++;
    }

    free(input_copy);
    return 0;
}



struct msh_pipeline *
msh_sequence_pipeline(struct msh_sequence *s)
{
    if (s->pl_count > 0) {
        struct msh_pipeline* temp = s->pipelines[0];
        int i;
        for (i = 0; i < (int) s->pl_count - 1; i++) {
            s->pipelines[i] = s->pipelines[i + 1];
        }
        s->pl_count--;

        return temp;
    } else {
        return NULL;
    }
}



struct msh_command *msh_pipeline_command(struct msh_pipeline *p, size_t nth)
{
    if (p == NULL) {
        return NULL;
    }

    if (nth >= p->cmd_count) {
        return NULL;
    }

    return p->commands[nth];
}



int msh_pipeline_background(struct msh_pipeline *p)
{
    if (p == NULL) {
        return 0;
    }
    if (p->background) {
        return 1;
    }
    return 0;
}
int msh_command_final(struct msh_command *c)
{
    if (c == NULL) {
        return 0;
    }

    if (c->last_cmd) {
        return 1;
    }

    return 0;
}
void
msh_command_file_outputs(struct msh_command *c, char **stdout, char **stderr)
{
	(void)c;
	(void)stdout;
	(void)stderr;
}

char *msh_command_program(struct msh_command *c)
{
    if(c && c -> args_count > 0){
        return c -> args[0];
    }
    return NULL;
}
void msh_command_putdata(struct msh_command *c, void *data, msh_free_data_fn_t fn) {
    // Free existing data if present and free function is provided
    if (c->pid && c->freefn) {
        c->freefn(c->pid);
    }

    c->pid = data; // Assign new data
    c->freefn = fn; // Assign function to free the data
}

void * msh_command_getdata(struct msh_command *c) {
    return c->pid; // Return the associated data
}


char **msh_command_args(struct msh_command *c) {
    if (c == NULL || c->args == NULL) {
        return NULL; 
    }
    return c->args;
}