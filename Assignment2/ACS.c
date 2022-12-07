/*
Name: Franco Cai
Vid: V00940471
Subject: CSC 360 A2
*/

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h> // for time calculation
#include <unistd.h> 
#include <assert.h> // for assert() in queue

#define MAX_LEN_LINE 50
#define MAX_CUST 100
#define INITIAL_CLERCK 5
#define TRUE 1
#define FALSE 0 
#define BUSY 1
#define IDLE 0

// Struct used to store information read in customer.txt
typedef struct customer_info { 
    int user_id;
	int class_type;
	double service_time;
	double arrival_time;
    struct customer_info* next;
}customer_info;

// Struct used to store clerck information and update for clerck_entry
typedef struct clerck_info {
    int user_id;
    int clerck_status; //The status of the clerck, 0 for IDLE and 1 for BUSY
    struct clerck_info* next;
}clerck_info;

// Queue structure
typedef struct cust_queue {
    customer_info* head;
    customer_info* tail;
}cust_queue;

typedef struct clerck_queue{
    clerck_info* head;
    clerck_info* tail;
}clerck_queue;

// Global variables 
struct timeval init_time; // initial time for simulated time
double cust_wait_time, busi_wait_time, econ_wait_time; // average waiting time for customers in each queue
double business_num, economic_num; // The count of the 
int queue_length[2]; // represent the lenght of each queue, 0 for economic class and 1 for business class 
int clerck_status[INITIAL_CLERCK]; // The status of each clerck from 0-4
pthread_cond_t serving_cond;
pthread_mutex_t bLock, eLock, start_time_mtex; 
cust_queue business, economic; // Queue for two customer classes
clerck_queue clerckQ;

//------------------------
//Queue implementation

void cust_QueueInit(cust_queue* pointer) {
    assert(pointer);
    pointer->head = pointer->tail = NULL;
}

void clerck_QueueInit(clerck_queue* pointer) {
    assert(pointer);
    pointer->head = pointer->tail = NULL;
}

void EnQueue(cust_queue* Qpointer, customer_info* customer) {
    customer->next = NULL;
    if(Qpointer->head == NULL) {
        Qpointer->head = Qpointer->tail = customer;
    } else {
        Qpointer->tail->next = customer;
        Qpointer->tail = customer;
    }
}

void clerckEnQueue(clerck_queue* Qpointer, clerck_info* customer) {
    customer->next = NULL;
    if(Qpointer->head == NULL) {
        Qpointer->head = Qpointer->tail = customer;
    } else {
        Qpointer->tail->next = customer;
        Qpointer->tail = customer;
    }
}

clerck_info* clerck_DeQueue(clerck_queue* Qpointer) {
    assert(Qpointer);
    assert(Qpointer->head);
    clerck_info* temp = Qpointer->head;
    Qpointer->head = Qpointer->head->next;
    if(Qpointer->head == NULL) {
        Qpointer->tail = NULL;
    }
    return temp;
}
void cust_DeQueue(cust_queue* Qpointer) {
    assert(Qpointer);
    assert(Qpointer->head);
    Qpointer->head = Qpointer->head->next;
    if(Qpointer->head == NULL) {
        Qpointer->tail = NULL;
    }
}

int cust_isEmpty(cust_queue* Qpointer) {
    assert(Qpointer);
    return Qpointer->head == NULL ? 1 : 0;
}

int clerck_isEmpty(clerck_queue* Qpointer) {
    assert(Qpointer);
    return Qpointer->head == NULL ? 1 : 0;
}

int isHead(cust_queue* Qpointer, customer_info* customer) {
    assert(Qpointer);
    return Qpointer->head == customer ? TRUE : FALSE;
}

//------------------------
//General helper functions 

// read customer information from the input file
int read_file(char* filename, customer_info* customer) {

    int total_cust;
    int counter = 0;
    int cust_id, class, serv_time, arriv_time; 
    char buffer[MAX_LEN_LINE];

    FILE* cust_file = fopen(filename,"r");

    if(cust_file == NULL) {
        printf("Open file failed.\n");
        exit(1);
    }
    //Get the number number of total customer from the first line of file
    fgets(buffer, 20,cust_file);
    total_cust = atoi(buffer);

    //get information and store it into the customer array
    while(!feof(cust_file)) {
        fgets(buffer,20,cust_file);
        sscanf(buffer,"%d:%d,%d,%d", &cust_id, &class, &arriv_time, &serv_time);
        if(arriv_time <= 0 || serv_time <= 0) {
            printf("The customer %d has an illegal value.\n", cust_id);
            continue;
        }
        customer[counter].user_id = cust_id;
        customer[counter].class_type = class;
        customer[counter].service_time = serv_time/10.0;
        customer[counter].arrival_time = arriv_time/10.0;
        counter++;
        if (class == 1){
            business_num++;
        } else {
            economic_num++;
        }
    }
    fclose(cust_file);
    
    return total_cust;
}

double getCurrentSimulationTime(){

	struct timeval cur_time;
	double cur_secs, init_secs;
	
	pthread_mutex_lock(&start_time_mtex);
	init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
	pthread_mutex_unlock(&start_time_mtex);
	
	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
	
	return cur_secs - init_secs;
}

// Check if theire is a IDLE clerck, if there is return the id of clerck, otherwise return -1;
int checkStatus() {
    for(int i = 0;i < INITIAL_CLERCK; i++){
        if (clerck_status[i] == IDLE) {
            return i;
        }
    }
    return -1;
}

//--------------------------------------------
//Thread functions and thread helper functions

void * customer_entry(void * customer) {

    customer_info * my_info = (customer_info *) customer;
    clerck_info* servingClerck;
    double queue_enter_time;
    int cur_clerck = 0;

    usleep(my_info->arrival_time * 1000000); // usleep takes a parameter of usec, so requires (* 1000000)
    fprintf(stdout, "A customer arrives: customer ID %2d. \n", my_info->user_id);
    int cur_queue = my_info->class_type == 0 ? 0 : 1;

    pthread_mutex_lock((my_info->class_type == 0) ? &eLock : &bLock); {

        queue_enter_time = getCurrentSimulationTime();
        EnQueue(cur_queue == 0 ? &economic : &business,my_info);
        queue_length[cur_queue]++;
        fprintf(stdout, "Customer %d enters a queue: the queue ID %1d, and length of the queue %2d. \n", my_info->user_id, cur_queue, queue_length[cur_queue]);

        while (TRUE) {
            if(!cust_isEmpty(&business)){
            usleep(1);
            }
            while(clerck_isEmpty(&clerckQ)) {
                pthread_cond_wait(&serving_cond,(my_info->class_type == 0) ? &eLock : &bLock);
            }
 			if (isHead(cur_queue == 0 ? &economic : &business, my_info) || 1) {
				cust_DeQueue(cur_queue == 0 ? &economic : &business);
                servingClerck = clerck_DeQueue(&clerckQ);
				queue_length[cur_queue]--;
				break;
			}
        }
    }
    pthread_mutex_unlock((my_info->class_type == 0) ? &eLock : &bLock);

    double start_serving_time = getCurrentSimulationTime();
    cust_wait_time += (start_serving_time - queue_enter_time);
    if(cur_queue == 0) {
        econ_wait_time += (start_serving_time - queue_enter_time);
    } else {
        busi_wait_time += (start_serving_time - queue_enter_time);
    } 

    fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n",start_serving_time, my_info->user_id, servingClerck->user_id);
    usleep(my_info->service_time * 1000000);
    double finish_serving_time = getCurrentSimulationTime();
    fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n",finish_serving_time, my_info->user_id, servingClerck->user_id);
    clerckEnQueue(&clerckQ, servingClerck);
    pthread_cond_signal(&serving_cond);

    pthread_exit(0); 
}

int main(int argc,char* argv[]) {
    
    if(argc != 2) {
        printf("Invalid input, please input a file!\n");     
        exit(0);
    }

    int total_customer;
    customer_info customer[MAX_CUST];
    clerck_info clerck[INITIAL_CLERCK];
    cust_QueueInit(&business); // Initialize the queue for business class
    cust_QueueInit(&economic); // Initialize the queue for economic class
    clerck_QueueInit(&clerckQ); // Initialize the queue for av
    gettimeofday(&init_time,NULL); // Initialize the start time for time count

    for(int i = 0; i < INITIAL_CLERCK; i++) {
        clerck[i].user_id = i;
        clerck[i].clerck_status = IDLE;
        clerckEnQueue(&clerckQ, &clerck[i]);
    }

    char* filename = argv[1];
    total_customer = read_file(filename, customer);
    
    pthread_t customer_thread[total_customer];
    pthread_t clerck_thread[INITIAL_CLERCK];
    
    for(int i = 0; i< INITIAL_CLERCK; i++){
        clerck_status[i] = IDLE;
    }
 
    //Initialize the mutex
    int mutex_check1 = pthread_mutex_init(&bLock,NULL);
    int mutex_check2 = pthread_mutex_init(&eLock,NULL);
    int mutex_check5 = pthread_mutex_init(&start_time_mtex,NULL);

    if(mutex_check1 || mutex_check2 || mutex_check5 != 0) {
       printf("Mutex initialize failed.\n");
    }

    //Initialize the condition variables
    int cond_init_check1 = pthread_cond_init(&serving_cond,NULL);

    if (cond_init_check1 != 0) {
        printf("Conditional variable intialize failed.\n");
    }

    //Create threads for each customer based on the customer id
    for(int i = 0; i < total_customer; i++) {
        pthread_create(&customer_thread[i], NULL, customer_entry, (void*) &customer[i]);
    }

    //Wait for all thread to be joined to the main thread
    for(int i = 0; i < total_customer; i++) {
        pthread_join(customer_thread[i],NULL);
    }

    //Destory the mutexes
    int mutex_dest_check1 = pthread_mutex_destroy(&bLock);
    int mutex_dest_check2 = pthread_mutex_destroy(&eLock);
    int mutex_dest_check5 = pthread_mutex_destroy(&start_time_mtex);

    if(mutex_dest_check1 || mutex_dest_check2 || mutex_dest_check5 != 0) {
       printf("Mutex destory failed.\n");
    }

    //Destroy the condition variables
    int cond_dest_check1 = pthread_cond_destroy(&serving_cond);

    if (cond_dest_check1 != 0) {
        printf("Failed to destory one of the condition variables.\n");
    }

    //Print the standard output for overall serving time
    cust_wait_time = cust_wait_time/total_customer;
    busi_wait_time = busi_wait_time/business_num;
    econ_wait_time = econ_wait_time/economic_num;
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n",cust_wait_time);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n",busi_wait_time);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",econ_wait_time);
    
    return 0;
}