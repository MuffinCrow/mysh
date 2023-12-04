#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>

struct cmd_Node
{
    char* cmd; //Command to execute
    char* input;
    char* output;
    char** arguments; //List of Arguments inputted
    int num_args;
    int then_else; // 0 - NONE ; 1 - then; 2 - else
    int executed; // 0 - False; 1 - True
    struct cmd_Node* next_Node;
    struct cmd_Node* prev_Node;
};

char cwd[4096];

int cwdGrabber() {
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error obtaining cwd.\n");
        return 0; //Did not run
    }
    //printf("Working Directory: %s\n", cwd);
    return 1; //Executed successfully
}

void changeDirectory(struct cmd_Node* node) {
    if (node->num_args == 0) {
        int check = chdir(getenv("HOME"));
        if (check != 0) {
            write(1, "Error: chdir() failed to get to home directory.\n", sizeof("Error: chdir() failed to get to home directory.\n"));
            return;
        }
        node->executed = 1;
    } else if (strncmp(node->arguments[0], "~", 1) == 0) {
        char* home = getenv("HOME");
        if (home == NULL) {
            write(1, "Error: Could not obtain home directory.\n", sizeof("Error: Could not obtain home directory.\n"));
            return;
        }

        if (sizeof(node->arguments[0]) > 1) {
            char tempPath[4096];
            strcpy(tempPath, node->arguments[0] + 1);
            snprintf(cwd, 4096, "%s%s", home, tempPath);
        } else {
            strcpy(cwd, home);
        }

        if (chdir(cwd) != 0) {
            write(1, "Error: Could not path from home directory.\n", sizeof("Error: Could not path from home directory.\n"));
            return;
        }
        node->executed = 1;
    } else {
        if (chdir(node->arguments[0]) != 0) {
            write(1, "Error: cd could not find given path.\n", sizeof("Error: cd could not find given path.\n"));
            return;
        }
        node->executed = 1;
    }
}

void commandExec (struct cmd_Node* node) {
    if  (node->cmd == NULL) {
        printf("NULL command given.\n");
    } else if (strcmp(node->cmd, "cd") == 0) {
        if (node->num_args < 2) {
            changeDirectory(node);
        } else {
            write(1, "Error: cd provided too many arguments.\n", sizeof("Error: cd provided too many arguments.\n"));
        }
    } else if (strcmp(node->cmd, "pwd") == 0) {
        int check = 0;
        if ((node->prev_Node != NULL) && (node->then_else != 0)) { //For uses of then or else
            if (((node->prev_Node->executed == 1) && (node->then_else == 1)) || ((node->prev_Node->executed == 0) && (node->then_else == 2))) {
                int fd = 1;
                if (node->output != NULL) {
                    fd = open(node->output, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    if (fd == -1) {
                        printf("Error: Failed to open output file.\n"); 
                        return;
                    }
                }
                check = cwdGrabber();
                write(fd, cwd, strlen(cwd));
                printf("\n");
                if (fd > 2) {close(fd);}
                node->executed = check;
            }
        } else if (node->then_else == 2) {
            printf("Error: No previous execution.\n");
            return;
        } else {
            int fd = 1;
            if (node->output != NULL) {
                fd = open(node->output, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (fd == -1) {
                    printf("Error: Failed to open output file.\n"); 
                    return;
                }
            }
            check = cwdGrabber();
            write(fd, cwd, strlen(cwd));
            printf("\n");
            if (fd > 2) {close(fd);}
            node->executed = check;
        }
    } else if (strcmp(node->cmd, "which") == 0) {
        //which call
        printf("which\n");
    } else if (strchr(node->cmd, '/')) {
        if (node->prev_Node != NULL) {
            if ((node->prev_Node->executed == 1 && node->then_else == 2) || (node->prev_Node->executed == 0 && node->then_else == 1)) {
                return;
            }
        }
        if (node->input != NULL || node->output != NULL) {
            pid_t pid = fork();

            if (pid == -1) {
                printf("Error: Failed to fork for redirection.\n");
                return;
            } else if (pid == 0) {
                int inputFile;
                int outputFile;
                if (node->input != NULL) {
                    inputFile = open(node->input, O_RDONLY);
                    if (inputFile == -1) {
                        printf("Error: Could not open input file.\n");
                        return;
                    }
                    
                    if (dup2(inputFile, STDIN_FILENO) == -1) {
                        printf("Error: Could not redirect from input file.\n");
                        close(inputFile);
                        return;
                    }
                    close(inputFile);
                }
                
                if (node->output != NULL) {
                    outputFile = open(node->output, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    if (outputFile == -1) {
                        printf("Error: Could not open output file.\n");
                        return;
                    }

                    if (dup2(outputFile, STDOUT_FILENO) == -1) {
                        printf("Error: Could not redirected to output file.\n");
                        close(outputFile);
                        return;
                    }
                    close(outputFile);
                }

                char** newArgs = (char**)malloc(sizeof(char*) * (node->num_args + 1));
                if (newArgs == NULL) {
                    printf("Error: Failed to create memory for first WD args.\n");
                    return;
                }

                newArgs[0] = (char*)malloc(sizeof(char) * sizeof(node->cmd));
                strcpy(newArgs[0], node->cmd);

                for (int i = 1; i < node->num_args; i++)
                {
                    newArgs[i] = (char*)malloc(sizeof(char) * sizeof(node->arguments[i-1]));
                    if (newArgs[i] == NULL) {
                        printf("Error: Failed to create memory for WD args.\n");
                        return;
                    }
                    strcpy(newArgs[i], node->arguments[i-1]);
                }

                int errorCheck = execv(node->cmd, node->arguments);
                if (errorCheck == -1) {
                    printf("Error: Could not execute command with redirects.\n");
                    return;
                }
                node->executed = 1;
            } else {
                wait(NULL);
            }
        } else {
            char** newArgs = (char**)malloc(sizeof(char*) * (node->num_args + 1));
            if (newArgs == NULL) {
                printf("Error: Failed to create memory for first WD args.\n");
                return;
            }

            newArgs[0] = (char*)malloc(sizeof(char) * sizeof(node->cmd));
            strcpy(newArgs[0], node->cmd);

            for (int i = 1; i < node->num_args; i++)
            {
                newArgs[i] = (char*)malloc(sizeof(char) * sizeof(node->arguments[i-1]));
                if (newArgs[i] == NULL) {
                    printf("Error: Failed to create memory for WD args.\n");
                    return;
                }
                strcpy(newArgs[i], node->arguments[i-1]);
            }

            int errorCheck = execv(node->cmd, node->arguments);
            if (errorCheck == -1) {
                printf("Error: Could not execute command.\n");
                return;
            }
            node->executed = 1;
        }
    } else {
        if (node->prev_Node != NULL) {
            if ((node->prev_Node->executed == 1 && node->then_else == 2) || (node->prev_Node->executed == 0 && node->then_else == 1)) {
                return;
            }
        }
        char filepath[256];
        char filepath1[256];
        char filepath2[256];
        char filepath3[256];
        int whichPath = 0;

        snprintf(filepath1, sizeof(filepath1), "/usr/local/bin/%s", node->cmd);
        snprintf(filepath2, sizeof(filepath2), "/usr/bin/%s", node->cmd);
        snprintf(filepath3, sizeof(filepath3), "/bin/%s", node->cmd);

        if (access(filepath1, F_OK | X_OK) != -1) {
            whichPath = 1;
            strcpy(filepath, filepath1);
        } else if (access(filepath2, F_OK | X_OK) != -1) {
            whichPath = 2;
            strcpy(filepath, filepath2);
        } else if (access(filepath3, F_OK | X_OK) != -1) {
            whichPath = 3;
            strcpy(filepath, filepath3);
        }

        if (whichPath == 0) {
            printf("Error: Command not found.\n");
            return;
        }

        // **************************************************************************************************************************************************************
        if (node->input != NULL || node->output != NULL) {
            pid_t pid = fork();

            if (pid == -1) {
                printf("Error: Failed to fork for redirection.\n");
                return;
            } else if (pid == 0) {
                int inputFile;
                int outputFile;
                if (node->input != NULL) {
                    inputFile = open(node->input, O_RDONLY);
                    if (inputFile == -1) {
                        printf("Error: Could not open input file.\n");
                        return;
                    }
                    
                    if (dup2(inputFile, STDIN_FILENO) == -1) {
                        printf("Error: Could not redirect from input file.\n");
                        close(inputFile);
                        return;
                    }
                    close(inputFile);
                }
                
                if (node->output != NULL) {
                    outputFile = open(node->output, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    if (outputFile == -1) {
                        printf("Error: Could not open output file.\n");
                        return;
                    }

                    if (dup2(outputFile, STDOUT_FILENO) == -1) {
                        printf("Error: Could not redirected to output file.\n");
                        close(outputFile);
                        return;
                    }
                    close(outputFile);
                }

                char** newArgs = (char**)malloc(sizeof(char*) * (node->num_args + 1));
                if (newArgs == NULL) {
                    printf("Error: Failed to create memory for first WD args.\n");
                    return;
                }

                newArgs[0] = (char*)malloc(sizeof(char) * sizeof(filepath));
                strcpy(newArgs[0], filepath);

                for (int i = 1; i < node->num_args; i++)
                {
                    newArgs[i] = (char*)malloc(sizeof(char) * sizeof(node->arguments[i-1]));
                    if (newArgs[i] == NULL) {
                        printf("Error: Failed to create memory for WD args.\n");
                        return;
                    }
                    strcpy(newArgs[i], node->arguments[i-1]);
                }

                int errorCheck = execv(filepath, node->arguments);
                if (errorCheck == -1) {
                    printf("Error: Could not execute command with redirects.\n");
                    return;
                }
                node->executed = 1;
            } else {
                wait(NULL);
            }
        } else {
            char** newArgs = (char**)malloc(sizeof(char*) * (node->num_args + 1));
            if (newArgs == NULL) {
                printf("Error: Failed to create memory for first WD args.\n");
                return;
            }

            newArgs[0] = (char*)malloc(sizeof(char) * sizeof(filepath));
            strcpy(newArgs[0], filepath);

            for (int i = 1; i < node->num_args; i++)
            {
                newArgs[i] = (char*)malloc(sizeof(char) * sizeof(node->arguments[i-1]));
                if (newArgs[i] == NULL) {
                    printf("Error: Failed to create memory for WD args.\n");
                    return;
                }
                strcpy(newArgs[i], node->arguments[i-1]);
            }

            int errorCheck = execv(filepath, node->arguments);
            if (errorCheck == -1) {
                printf("Error: Could not execute command.\n");
                return;
            }
            node->executed = 1;
        }
        // **************************************************************************************************************************************************************
    }

}

void pipeORexec (struct cmd_Node* node) {
    if (node == NULL) {
        printf("NULL node executed.\n");
        return;
    }

    if (node->next_Node == NULL) { //the else is for piping
        commandExec(node);
    } else {
        struct cmd_Node* secnode = node->next_Node;
        
        if (secnode->then_else != 0) {
            printf("Error: Cannot have then or else in the second command of a pipe.\n");
            return;
        }

        // **********************************************************************************************************************************************************************************************
        if (node->output != NULL || secnode->input != NULL) {
            if (node->output != NULL) {
                commandExec(node);
                commandExec(secnode);
            } else {
                pid_t pid;

                pid = fork();

                if (pid == -1) {
                    printf("Error: Failed to create fork while piping and having redirects.\n");
                    return;
                }

                if (pid == 0) {
                    int devNull = open("/dev/null", O_WRONLY);
                    dup2(devNull, STDOUT_FILENO);
                    close(devNull);

                    commandExec(node);
                } else {
                    wait(NULL);

                    if (node->executed == 1) {
                        commandExec(secnode);
                    } else {
                        printf("Error: First command failed to run. Second command terminated.\n");
                        return;
                    }
                }
            }
        } else { // **********************************************************************************************************************************************************************************************
            int pipefd[2];
            pid_t pid1, pid2;

            if (pipe(pipefd) == -1) {
                printf("Error: Failed to pipe.\n");
                return;
            }

            if ((pid1 = fork()) == -1) {
                printf("Error: Failed for fork for pid1.\n");
                return;
            }

            if (pid1 == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                commandExec(node);
            }

            if ((pid2 = fork()) == -1) {
                printf("Error: Failed for fork for pid1.\n");
                return;
            }

            if (pid2 == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                commandExec(secnode);
            }

            close(pipefd[0]);
            close(pipefd[1]);

            wait(NULL); wait(NULL);
        }
        // **********************************************************************************************************************************************************************************************
    }
}

void wildcards(char* split_line, char*** arg_array, int* num_args, int* array_size){
    glob_t glob_result;
    int i;

    if(glob(split_line, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result) == 0){
        //We don't know if the size of the array is large enough to handle all the args being added to the array
        while(*num_args + glob_result.gl_pathc > *array_size){
            *array_size *= 2;
            char** temp = realloc(*arg_array, sizeof(char*) * (*array_size));
            if(temp == NULL){
                printf("Error in reallocating arg_array\n");
                exit(EXIT_FAILURE);
            }
            *arg_array = temp;
        }
        for(i = 0; i < glob_result.gl_pathc && *num_args < *array_size; i++){
            (*arg_array)[(*num_args)++] = strdup(glob_result.gl_pathv[i]);
        }
    }
    globfree(&glob_result);
}

struct cmd_Node* create_Node(char* line){
    struct cmd_Node* node_A = (struct cmd_Node*)malloc(sizeof(struct cmd_Node));
    const char* space = " ";
    char* split_line;
    int length_line = strlen(line);
    int temparr_size = 5;
    char** copy_arguments = malloc(sizeof(char*) * temparr_size);
    int pipe_found = 0;

    if (copy_arguments == NULL){
        printf("Malloc Failed - copy_arguments\n");
    }

    //I tested this but strtok(line,space) will get us individual args separated by spaces
    //The first call will actually be set to the first word, each following call will be set to the next word
    split_line = strtok(line, space);

    if ((strcmp(split_line, "then") == 0 )){
        node_A->then_else = 1;
        node_A->cmd = strtok(NULL, space);
    } else if ((strcmp(split_line, "else") == 0 )){
        node_A->then_else = 2;
        node_A->cmd = strtok(NULL, space);
    } else {
        node_A->then_else = 0;
        node_A->cmd = strdup(split_line);
    }

    node_A->num_args = 0;
    copy_arguments[node_A->num_args++] = node_A->cmd;

    while((split_line = strtok(NULL, space)) != NULL){
        
        if(strcmp(split_line, "|") == 0){
            pipe_found = 1;
            break;
        }
        if (strcmp(split_line, "<") == 0){
            node_A->input = strtok(NULL, space);
            continue;
        }
        if (strcmp(split_line, ">") == 0){
            node_A->output = strtok(NULL, space);
            continue;
        }
        if (strchr(split_line, '*')){
            wildcards(split_line, &copy_arguments, &node_A->num_args , &temparr_size);
            continue;
        }
        if (node_A->num_args == temparr_size){
            temparr_size *= 2;
            char** temp = realloc(copy_arguments, sizeof(char*) * temparr_size);
            if(temp == NULL){
                printf("Error in reallocating- temp\n");
            }else{
                copy_arguments = temp;
            }
        }
        copy_arguments[node_A->num_args++] = strdup(split_line);
    }
    
    node_A->arguments = malloc(sizeof(char*) * (node_A->num_args + 1));//NULL pointer at the end of args array
    if (node_A->arguments == NULL){
        printf("Malloc Failed - node_A arguments\n");
    }
    for(int i = 0; i < node_A->num_args; i++){
        node_A->arguments[i] = copy_arguments[i];
    }
    free(copy_arguments);
    node_A->arguments[node_A->num_args] = NULL;
    node_A->executed = 0;
    node_A->next_Node = NULL;
    node_A->prev_Node = NULL;
    //Recursive call for piping in case we want to do dynamic piping.
    if (pipe_found == 1){
        char* new_line = malloc(length_line + 1);
        new_line[0] = '\0';

        while((split_line = strtok(NULL, space)) != NULL){
            strcat(new_line, split_line);
            strcat(new_line, " ");
        }
        struct cmd_Node* pipe_node = create_Node(new_line);
        node_A->next_Node = pipe_node;
        pipe_node->prev_Node = node_A;
        free(new_line);
    }
    

    return node_A;
}

void mode_Loop(int flag, char* file_name){
    struct cmd_Node* head = NULL;
    struct cmd_Node* tail = NULL;
    char* line = NULL;
    size_t len = 0;
    FILE* fp = NULL;

    if (flag == 0){
        printf("Welcome to my shell!\n");
    } else {
        fp = fopen(file_name, "r");

        if (fp == NULL){
                printf("No such file\n");
                exit(EXIT_FAILURE);
            }
    }

    while(1){
        if (flag == 0){
            printf("mysh> ");
            if(getline(&line, &len, stdin) > 0){
                if (strcmp(line, "exit") == 0){
                    printf("mysh: exiting\n");
                    break;
                } else {
                    struct cmd_Node* node = create_Node(line);
                        if (head == NULL){
                            head = node;
                            tail = node;
                        } else {
                            node->prev_Node = tail;
                            tail = node;
                        }
                    commandExec(node);
                }
            } else {
                printf("Error occured reading a line - interactive\n");
            }
        } else {
            if (getline(&line, &len, fp) > 0){
                if (strcmp(line, "exit") == 0){
                    printf("mysh: exiting\n");
                    break;
                } else {
                    struct cmd_Node* node = create_Node(line);
                    if (head == NULL){
                        head = node;
                        tail = node;
                    } else {
                        node->prev_Node = tail;
                        tail = node;
                    }
                    commandExec(node);
                }
            } else {
                fclose(fp);
                break;
            }
        }
    }

    free(line);
    exit(EXIT_SUCCESS);
}

int main(int argc, char ** argv){
    // struct cmd_Node node1 = {0}, node2 = {0};
    // node1.cmd = "./your_command";
    //node2.cmd = NULL; // Setting cmd to NULL
    // node1.next_Node = &node2;
    // node2.prev_Node = &node1;
    // node2.cmd = "pwd";
    // node1.cmd = "ls";
    // char* path = "~/cs214";
    // node1.arguments = malloc(1 * sizeof(path));
    // node1.arguments[0] = path;
    // node1.num_args = 1;
    // node1.output = "output.txt";

    //commandExec(&node1);
    //commandExec(&node2);
    //commandExec(&node3);

    // if (argc > 2){
    //     printf("mysh.c takes up to one argument");
    //     }

    // if (argc == 2){
    //     mode_Loop(1, argv[1]);
    // } else {
    //     mode_Loop(0, NULL);
    // }

    // return 0;

    // char cwd[4096];
    // strcpy(cwd, getWorkingDirectory());
    // printf("Working Directory: %s\n", cwd);
    // cwdGrabber();
    // // printf("Length: %ld\n", strlen(cwd));
    // printf("Working Directory: %s\n", cwd);
    // changeDirectory(&node1);
    // cwdGrabber();
    // printf("Working Directory: %s\n", cwd);
    // commandExec(&node1);
    // pipeORexec(&node1);
    // return 0;
}
