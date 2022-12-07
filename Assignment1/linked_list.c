#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sigqnal.h>
#include <errno.h>

#include "linked_list.h"

 
Node * add_newNode(Node* head, pid_t new_pid, char * new_path){
	Node* new_node = (Node*) malloc(sizeof(Node));
	new_node->pid  = new_pid;
	new_node->path = new_path;
	new_node -> next = head;
	return new_node;
}


Node * deleteNode(Node* head, pid_t pid){
	// your code here
	Node* cur = head;
	Node* pre = head;

	while(cur->next != NULL && cur->pid != pid) {
		pre = cur;
		cur = cur->next;
	}
	if (cur->pid == pid) {
		if(cur == head) {
			head = cur->next;
		} else {
			pre->next = cur->next;
		}
		free(cur);
		cur = NULL;
	}
	return head;
}

void printList(Node *node){
	// your code here
	int i = 0;
	Node* cur = node;
	while (cur != NULL) {
		printf("%d: %s\n",cur->pid, cur ->path);
		cur = cur ->next;
		i++;
	}
	printf("Total Background jobd: %d\n", i);
	}


int PifExist(Node *node, pid_t pid){
	// your code here
	Node * cur = node;
	while(cur != NULL) {
		if(cur->pid == pid){
			return 1;
		}
		cur = cur->next;
	}
  	return 0;
}
