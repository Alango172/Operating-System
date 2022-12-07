#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"

Node* head = NULL;

/*Crate a background process with inserted command
  and add it to the list
  **cmd is the command inserted
*/
void func_BG(char **cmd){
  	//Your code here;
    pid_t pid; 
    pid = fork();
    if(pid == 0) {
      //child process, use execvp
      char* command = cmd[1];
      execvp(command, &cmd[1]);
      exit(1);
    } else if(pid > 0) {
      // Parent process, add it to the list
      head = add_newNode(head,pid,"/home/chaofancai/A1");
      printf("Process %d created\n", pid);
      sleep(1);
    } else {
      printf("Fork failed.\n");
      return;
    }
    return;
}

// Print the list of process created
void func_BGlist(char **cmd){
	//Your code here;
    printList(head);
}

/* Kill the process with inserted pid
   with sigterm amd delete the node from 
   the process list
*/
void func_BGkill(char * str_pid){
	//Your code here
  pid_t pId = atoi(str_pid);
  int exist = PifExist(head,pId);
  if(exist) {
    int success = kill(pId,SIGTERM);
    if(success == 0) {
      head = deleteNode(head, pId);
      printf("Kill process successfuly.\n");
      return;
    } else {
      printf("Unable to terminate the process\n");
    }
  } else {
    printf("Error: Process %d does not exist\n",pId);
  }
}

// Hang the process with inserted pid
void func_BGstop(char * str_pid){
	//Your code here
  pid_t pId = atoi(str_pid);
  int exist = PifExist(head,pId);
  if(exist) {
    int success = kill(pId,SIGSTOP);
    if(success == 0) {
      printf("Stop process successfuly.\n");
      return;
    } else {
      printf("Unable to stop the process\n");
    }
  } else {
    printf("Error: Process %d does not exist\n",pId);
  }
}

// Restart the process from stopped status
void func_BGstart(char * str_pid){
	//Your code here
  int pId = atoi(str_pid);
  int exist = PifExist(head,pId);
  if(exist) {
    int success = kill(pId,SIGCONT);
    if(success == 0) {
      printf("Start process successfuly.\n");
      return;
    } else {
      printf("Unable to continue the process\n");
    }
  } else {
    printf("Error: Process %d does not exist\n",pId);
  }
}

/* Print the information gain from 
   /proc/pid/status and /prco/pid/stat
*/
void func_pstat(char * str_pid){
	//Your code here
  int pid = atoi(str_pid);
  int exist = PifExist(head,pid);
  if(exist) {
    char stat[64];
    char status[64];
    char content[60][512]; 
    sprintf(stat,"/proc/%d/stat", pid);
    sprintf(status,"/proc/%d/status", pid);

    char comm[30];
    char state;
    int dArr[6]; //pid, ppid, pgrp, session, tty_nr, tgpid
    unsigned int flags;
    unsigned long int luArr[5]; // minflt, cminflt, majflt, cmajflt, vsize
    float utime, stime;
    long int ldArr[5]; // cutime, cstime, priority, nice, num_threads, itrealvalue
    long int rss;
    unsigned long long int starttime;

    FILE* ptStat = fopen(stat,"r");
    if(ptStat == NULL) {
      printf("File Does not exist\n");
      return;
    }
    fscanf(ptStat,"%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %f %f %ld %ld %ld %ld %ld %ld %llu %lu %ld", &dArr[0],comm, &state,&dArr[1],&dArr[2],
      &dArr[3],&dArr[4],&dArr[5],&flags,&luArr[0],&luArr[1],&luArr[2],&luArr[3],&utime,&stime, &ldArr[0],&ldArr[1],&ldArr[2],&ldArr[3],&ldArr[4]
      ,&ldArr[5],&starttime,&luArr[4],&rss);
    fclose(ptStat);

    unsigned long uTime = utime / sysconf(_SC_CLK_TCK);
    unsigned long sTime = stime / sysconf(_SC_CLK_TCK);

    printf("Comm:  %s\n",comm);
    printf("State:  %c\n", state);
    printf("utime:  %lu\n", uTime);
    printf("stime:  %lu\n", sTime);
    printf("rss:  %ld\n", rss);

    FILE* ptStatus = fopen(status,"r");
    if(ptStatus == NULL) {
      printf("File Does not exist\n");
      return;
    }
    int i = 0;

    while(!feof(ptStatus)) {
	 fgets(content[i],512,ptStatus);
	 i++;

    }
    char * volSwitch = content[53];
    char * nonVolSwitch = content[54];
    printf("%s", volSwitch);
    printf("%s", nonVolSwitch);

    //int volSwitch;
    //int nonVolSwitch;
    //fscanf(ptStatus, "voluntary_ctxt_switches:/t%d", &volSwitch);
    //fscanf(ptStatus, "nonvoluntary_ctxt_switches:/t%d", &nonVolSwitch);

    //printf("voluntary_ctxt_switches: %d\n", volSwitch);
    //printf("nonvoluntary_ctxt_switches: %d\n", nonVolSwitch);
    fclose(ptStatus);

  } else {
    printf("Error: Process %d does not exist\n",pid);
  }

}

/* Chech the zombie process everytime a command is inserted
   If the child process is already terminated, delete the process
   node from the process list
*/
void check_zombie_process() {
  int status;
  //Node* cur = head;
  while (true){
    // int opts = WUNTRACED|WCONTINUED|WNOHANG;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if(pid > 0) {
      if (WIFEXITED(status)) {
       	head = deleteNode(head, pid);
        printf("The process %d has been terminated\n", pid);
	return;
      }
    } else {
      break;
    }
    // if (pId > 0) {
    //   if (WIFEXITED(status)) {
    //     head = deleteNode(head, pId);
    //     printf("Process terminated%d", pId);  // Display the status code of child process
    //   } else if (WIFSTOPPED(status)) {
    //     printf("stopped by signal %d\n", WSTOPSIG(status));
    //   } else if (WIFCONTINUED(status)) {    
    //     head = add_newNode(head,pId,"/home/chaofancai/A1");
    //     printf("Process continued\n");   
    //   }
    // } else {
    //   break;
    // }
  }
}

int main(){
    char user_input_str[50];
    while (true) {
      printf("Pman: > ");
      fgets(user_input_str, 50, stdin);
      printf("User input: %s \n", user_input_str);
      char * ptr = strtok(user_input_str, " \n");
      if(ptr == NULL){
        continue;
      }
      char * lst[50];
      int index = 0;
      lst[index] = ptr;
      index++;
      while(ptr != NULL){
        ptr = strtok(NULL, " \n");
        lst[index]=ptr;
        index++;
      }
      if (strcmp("bg",lst[0]) == 0){
        func_BG(lst);
      } else if (strcmp("bglist",lst[0]) == 0) {
        func_BGlist(lst);
      } else if (strcmp("bgkill",lst[0]) == 0) {
        func_BGkill(lst[1]);
      } else if (strcmp("bgstop",lst[0]) == 0) {
        func_BGstop(lst[1]);
      } else if (strcmp("bgstart",lst[0]) == 0) {
        func_BGstart(lst[1]);
      } else if (strcmp("pstat",lst[0]) == 0) {
        func_pstat(lst[1]);
      } else if(strcmp(" ",lst[0]) == 0) {
        continue;
      }else if (strcmp("q",lst[0]) == 0) {
        printf("Bye Bye \n");
        exit(0);
      } else {
        printf("Invalid input\n");
      }
      check_zombie_process();
    }

  return 0;
}
