#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

typedef struct Node Node;

struct Node{
    pid_t pid;
    char * path;
    Node * next;
};


Node * add_newNode(Node* head, pid_t new_pid, char * new_path);
Node * deleteNode(Node* head, pid_t pid);
void printList(struct Node *node);
int PifExist(Node *node, pid_t pid);



#endif
