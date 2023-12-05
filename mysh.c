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
#include <ctype.h>


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

#define DEELIMIT_CHAR "<>|"

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

void cmd_Which(struct cmd_Node* node){
    if (node->num_args != 1){
        printf("Failed- given the wrong number of arguments\n");
        return;
    }
    char* prog = strdup(node->arguments[0]);
    char tempPath[2048];
    if (strchr(prog, '/') == NULL){
        const char* dir[] = {"/usr/local/bin", "/usr/bin", "/bin"};
        //For testing purposes
        //const char* dir[] = {"/common/home/adt85/cs214/Assignment3/usr/local/bin"};
        int size_dir = 3;

        for(int i = 0; i < size_dir; i++){
            snprintf(tempPath, sizeof(tempPath), "%s/%s", dir[i], prog);
            //printf("Checking: %s\n", tempPath);
            if(access(tempPath, F_OK) != -1){
                printf("%s\n", tempPath);
                return;
            }
        }
        printf("%s -Program not found\n", prog);
    } else if (access(prog, F_OK) == 0){
        printf("%s\n", prog);
    } else {
        printf("%s -Program not found\n", prog);
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
        cmd_Which(node);
        //printf("which\n");
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

                int errorCheck;
                pid_t pid = fork();
                if (pid == -1) {
                    printf("Error: Failed to fork before execution.\n");
                    return;
                }

                if (pid == 0) {
                    errorCheck = execv(node->cmd, node->arguments);
                } else {
                    wait(NULL);
                }
                
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
            int errorCheck;
            pid_t pid = fork();
            if (pid == -1) {
                printf("Error: Failed to fork before execution.\n");
                return;
            }

            if (pid == 0) {
                errorCheck = execv(node->cmd, node->arguments);
            } else {
                wait(NULL);
            }

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
                
                int errorCheck;
                pid_t pid = fork();
                if (pid == -1) {
                    printf("Error: Failed to fork before execution.\n");
                    return;
                }

                if (pid == 0) {
                    errorCheck = execv(filepath, node->arguments);
                } else {
                    wait(NULL);
                }

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

            int errorCheck;
            pid_t pid = fork();
            if (pid == -1) {
                printf("Error: Failed to fork before execution.\n");
                return;
            }

            if (pid == 0) {
                errorCheck = execv(filepath, node->arguments);
            } else {
                wait(NULL);
            }

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

char** parseTokens(const char* presplit_token, int* tokenCount){

    int size = 32;
    char** tokens = (char**)malloc(size * sizeof(char*));

    if (!tokens){
        printf("Malloc failed - Tokens\n");
        //Exit
    }

    *tokenCount = 0;

    const char* current = presplit_token;

    while(*current){

        while(isspace(*current)){
            current++;
        }

        if(!*current){
            break;
        }
        int length = 0;

        while (current[length] && !isspace(current[length]) && strchr(DEELIMIT_CHAR, current[length]) == NULL) {
            length++;
        }

        if (length > 0) {
            tokens[*tokenCount] = strndup(current, length);
            (*tokenCount)++;
        }

        current += length;

        if (strchr(DEELIMIT_CHAR, *current)) {
            tokens[*tokenCount] = strndup(current, 1);
            (*tokenCount)++;
            current++;
        }

        if (!*current) {
            break;
        }

        
        if (*tokenCount >= size) {
            size *= 2;
            char** temp = realloc(tokens, size * sizeof(char*));
            if (!temp) {
                printf("Realloc failed - parseTokens");
                for (int i = 0; i < *tokenCount; i++) {
                    free(tokens[i]);
                }
                free(tokens);
                exit(EXIT_FAILURE);
            }
            tokens = temp;
        }
    }

    return tokens;
}

void freeTokens(char** tokens, int tokenCount){
    for(int i = 0; i < tokenCount; i++){
        free(tokens[i]);
    }
    free(tokens);
}

struct cmd_Node* create_Node(char* line){
    struct cmd_Node* node_A = (struct cmd_Node*)malloc(sizeof(struct cmd_Node));
    node_A->num_args = 0;
    int length_line = strlen(line);
    int temparr_size = 32;
    char** copy_arguments = malloc(sizeof(char*) * temparr_size);
    int loop_pos = 1;
    int pipe_found = 0;
    
    int tokenCount;

    if (copy_arguments == NULL){
        printf("Malloc Failed - copy_arguments\n");
    }

    //I tested this but strtok(line,space) will get us individual args separated by spaces
    //The first call will actually be set to the first word, each following call will be set to the next word
    char** parsed_Tokens = parseTokens(strdup(line), &tokenCount);
    
    //Test to see if all tokens are read in correctly
    //for(int i = 0; i < tokenCount - 1; i++){
    //    printf("Token %d: %s\n", i, parsed_Tokens[i]);
    //}
    //For some reason when testing in a separate file it returns a blank token at the end so tokenCount - 1
    if(strcmp(parsed_Tokens[0], "then") == 0){
        node_A->then_else = 1;
        node_A->cmd = parsed_Tokens[1];
        loop_pos = 2;
    } else if(strcmp(parsed_Tokens[0], "else") == 0){
        node_A->then_else = 2;
        node_A->cmd = parsed_Tokens[1];
        loop_pos = 2;
    } else {
        node_A->cmd = parsed_Tokens[0];
        node_A->then_else = 0;
    }
    

    for(int i = loop_pos; i < tokenCount - 1; i++){
        if(strcmp(parsed_Tokens[i], "|") == 0){
            loop_pos = i + 1;
            pipe_found = 1;
            break;
        }
        if (strcmp(parsed_Tokens[i], "<") == 0){
            node_A->input = parsed_Tokens[++i];
            continue;
        }
        if (strcmp(parsed_Tokens[i], ">") == 0){
            node_A->output = parsed_Tokens[++i];
            continue;
        }
        if (strchr(parsed_Tokens[i], '*')){
            wildcards(parsed_Tokens[i], &copy_arguments, &node_A->num_args , &temparr_size);
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
        char* arg_dup = strdup(parsed_Tokens[i]);
        if(arg_dup == NULL){
            printf("Error in strdup\n");
        }
        //printf("Reached copy_arg\n");
        copy_arguments[node_A->num_args++] = arg_dup;
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

        for(int i = loop_pos; i < tokenCount - 1; i++){
            strcat(new_line, parsed_Tokens[i]);
            strcat(new_line, " ");
        }

        struct cmd_Node* pipe_node = create_Node(new_line);
        node_A->next_Node = pipe_node;
        pipe_node->prev_Node = node_A;
        free(new_line);
    }
    
    //printf("%d - num_args\n", node_A->num_args);
    //printf("%s - Arg-0\n", node_A->arguments[0]);
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
                line[strcspn(line, "\n")] = 0;
                if (strcmp(line, "exit") == 0){
                    printf("mysh: exiting\n");
                    break;
                } else if(line[0] == '\0'){
                    printf("Empty command entered.\n");
                } else {
                    struct cmd_Node* node = create_Node(line);
                    //printf("reached node creation\n");
                        if (head == NULL){
                            head = node;
                            tail = node;
                        } else {
                            node->prev_Node = tail;
                            tail = node;
                        }
                    pipeORexec(node);
                }
            } else {
                printf("Error occured reading a line - interactive\n");
            }
        } else {
            if (getline(&line, &len, fp) > 0){
                line[strcspn(line, "\n")] = 0;
                if (strcmp(line, "exit") == 0){
                    printf("mysh: exiting\n");
                    break;
                } else if(line[0] == '\0'){
                    printf("Empty command entered.\n");
                } else {    
                    struct cmd_Node* node = create_Node(line);
                    if (head == NULL){
                        head = node;
                        tail = node;
                    } else {
                        node->prev_Node = tail;
                        tail = node;
                    }
                    pipeORexec(node);
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

     if (argc > 2){
         printf("mysh.c takes up to one argument");
         }

     if (argc == 2){
         mode_Loop(1, argv[1]);
     } else {
         mode_Loop(0, NULL);
     }

    
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
     return 0;
}
