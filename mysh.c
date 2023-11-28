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
    char* cmd;
    char* input;
    char* output;
    char** arguments;
    int executed;
    struct cmd_Node* next_Node;
    struct cmd_Node* prev_Node;
    int then_else;
};

void commandExec (struct cmd_Node* node) {
    if (node == NULL) {
        printf("NULL node executed.\n");
        return;
    }

    while(node != NULL) {
        if  (node->cmd == NULL) {
            printf("NULL command given.\n");
        } else if (strcmp(node->cmd, "cd") == 0) {
            //cd call
            printf("cd\n");
        } else if (strcmp(node->cmd, "pwd") == 0) {
            //pwd call
            printf("pwd\n");
        } else if (strcmp(node->cmd, "which") == 0) {
            //which call
            printf("which\n");
        } else if (strncmp(node->cmd, "./", 2) == 0) {
            printf("Executes in WD\n");
        } else {
            printf("Executes in other directory\n");
        }
        
        if (node->next_Node == NULL) {
            printf("End loop\n");
            break;
        } else {
            printf("Next Node\n");
            node = node->next_Node;
        }
    }
}

int main (int argc, char *argv[]) {
    //struct cmd_Node node1 = {0}, node2 = {0}, node3 = {0};
    //node1.cmd = "./your_command";
    //node2.cmd = NULL; // Setting cmd to NULL
    //node1.next_Node = &node2;
    //node2.prev_Node = &node1;
    //node3.cmd = "pwd";

    //commandExec(&node1);
    //commandExec(&node2);
    //commandExec(&node3);
}