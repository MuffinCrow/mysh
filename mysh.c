#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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
        if (check != 0) write(1, "Error: chdir() failed to get to home directory.\n", sizeof("Error: chdir() failed to get to home directory.\n"));
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
    } else {
        if (chdir(node->arguments[0]) != 0) {
            write(1, "Error: cd could not find given path.\n", sizeof("Error: cd could not find given path.\n"));
        }
    }
}

void commandExec (struct cmd_Node* node) {
    if (node == NULL) {
        printf("NULL node executed.\n");
        return;
    }

    if (node->next_Node == NULL) { //the else is for piping
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
                        fd = open(node->output, O_WRONLY | O_CREAT | O_TRUNC);
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
                check = cwdGrabber();
                write(1, cwd, strlen(cwd));
                printf("\n");
                node->executed = check;
            }
        } else if (strcmp(node->cmd, "which") == 0) {
            //which call
            printf("which\n");
        } else if (strchr(node->cmd, '/')) {
            if (node->input != NULL || node->output != NULL) {

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

                execv(node->cmd, node->arguments);
            }
        } else {
            printf("Executes in other directory\n");
        }
    } else { //piping

    }
}

/**
 * Logic needs to be rewritten, I made a basic template but need to think about a few things
 * 1. Wildcards 2. Piping 3. How input output etc would work.
*/
struct cmd_Node* create_Node(char* line){
    struct cmd_Node* node_A = (struct cmd_Node*)malloc(sizeof(struct cmd_Node));
    const char* space = " ";
    char* split_line;
    //Need a variable sized array for arguments as we don't know it's size
    int temparr_size = 5;
    char** copy_arguments = malloc(sizeof(char*) * temparr_size);
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
        //split_line is a a char array not a char so we need to strcmp
        //add check for pipe symbol
        if (strcmp(split_line, "<") == 0){
            node_A->input = strtok(NULL, space);
            continue;
        }
        if (strcmp(split_line, ">") == 0){
            node_A->output = strtok(NULL, space);
            continue;
        }
        //Realloc when necessary, I do this after the strcmp lines as it doesnt matter(we don't add it to the array until after this code)
        if (node_A->num_args == temparr_size){
            temparr_size *= 2;
            char** temp = realloc(copy_arguments, sizeof(char*) * temparr_size);
            if(temp == NULL){
                printf("Error in reallocating- temp\n");
            }else{
                copy_arguments = temp;
            }
        }
        copy_arguments[node_A->num_args++] = split_line;
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

    return node_A;
}

void cmd_Parse(char* line){


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
                cmd_Parse(line);
            } else {
                printf("Error occured reading a line - interactive\n");
            }
        } else {
            if (getline(&line, &len, fp) > 0){
                cmd_Parse(line);
            } else {
                fclose(fp);
                break;
            }
        }
        struct cmd_Node* node = create_Node(line);
        if (head == NULL){
            head = node;
            tail = node;
        } else {
           tail->next_Node = node;
           tail = node;
        }
    }
    //At this point head will be a pointer to the start of the linked list of nodes


    free(line);
}

int main(int argc, char ** argv){
    struct cmd_Node node1 = {0};
    // node1.cmd = "./your_command";
    //node2.cmd = NULL; // Setting cmd to NULL
    //node1.next_Node = &node2;
    //node2.prev_Node = &node1;
    //node3.cmd = "pwd";
    // node1.cmd = "testfolder/testfile";
    // char* path = "~/cs214";
    // node1.arguments = malloc(1 * sizeof(path));
    // node1.arguments[0] = path;
    // node1.num_args = 1;

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
}
