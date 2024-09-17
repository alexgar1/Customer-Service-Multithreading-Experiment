#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "linkedls.h"

#define NQUEUE 2
#define NCLERKS 5
#define BUSINESS 1
#define ECONOMY 0
#define IDLE -1
#define MAXCUST 512

static struct timeval start_time;

// 2 queues
Node* head[NQUEUE];
int qlen[NQUEUE];
int ncust = 0; // number of customers
int finished = 0;
int custstat[MAXCUST];

int economytotal = 0;
double economytime = 0;
int businesstotal = 0;
double businesstime = 0;

// threads
pthread_t* custT;
pthread_t* clerkT;

// mutexes
pthread_mutex_t qmutex[NQUEUE];
pthread_mutex_t clerkmutex[NCLERKS];
pthread_mutex_t start_time_mtex;

// condvars
pthread_cond_t qcond[NQUEUE];
pthread_cond_t clerkcond[NCLERKS];

typedef struct {
    int id;
    int class;
    int arrivet;  // in 10ths of a second
    int svct;  // in 10ths of a second
} Customer;

// Function to read customers from a file
Customer* readCustomers(const char* filename, int* numCustomers) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // Read the number of customers from the first line
    fscanf(file, "%d\n", numCustomers);
    Customer* customers = malloc(*numCustomers * sizeof(Customer));
    if (!customers) {
        fclose(file);
        perror("Failed to allocate memory for customers");
        return NULL;
    }

    // Read each customer's details
    for (int i = 0; i < *numCustomers; i++) {
        fscanf(file, "%d:%d,%d,%d\n", 
               &customers[i].id, 
               &customers[i].class, 
               &customers[i].arrivet, 
               &customers[i].svct);

        if (customers[i].id!=i+1 || customers[i].class<0 || customers[i].class>1 || customers[i].arrivet<0 || customers[i].svct<0){
            fprintf(stdout, "Error in input file. Make sure customer ids count up (1,2,3...), all values are positive and class is either 0 or 1");
            exit(1);
        }
    }

    fclose(file);
    return customers;
}

double getCurrentSimulationTime(){
	
	struct timeval cur_time;
	double cur_secs, init_secs;
	
	//pthread_mutex_lock(&start_time_mtex); //you may need a lock here
	init_secs = (start_time.tv_sec + (double) start_time.tv_usec / 1000000);
	//pthread_mutex_unlock(&start_time_mtex);
	
	gettimeofday(&cur_time, NULL);
	cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
	
	return cur_secs - init_secs;
}

void* clerk(void* arg){
	int clerkID = *(int*) arg + 1;
    printf("Clerk %d is ready to help\n", clerkID);
    int helpingcust = -1;
    int sig;
    //usleep(100000);

    while (finished<ncust){
        sig = 0;
        //usleep(1000);
        pthread_mutex_lock(&qmutex[BUSINESS]);
        if (head[BUSINESS]!=NULL){
            helpingcust = head[BUSINESS]->id;
            
            if (custstat[helpingcust] == -1){ // if customer not already helped
                custstat[helpingcust] = clerkID;
                sig = 1;
            }         
            pthread_mutex_unlock(&qmutex[BUSINESS]);
            pthread_cond_signal(&qcond[BUSINESS]);

        }
        else {
            pthread_mutex_unlock(&qmutex[BUSINESS]);
            pthread_mutex_lock(&qmutex[ECONOMY]);
            if (head[ECONOMY]!=NULL){
                helpingcust = head[ECONOMY]->id;

                if (custstat[helpingcust] == -1){
                    custstat[helpingcust] = clerkID;
                    sig = 1;
                } 

                pthread_mutex_unlock(&qmutex[ECONOMY]);
                pthread_cond_signal(&qcond[ECONOMY]);
            }
            else {
                pthread_mutex_unlock(&qmutex[ECONOMY]);
                continue;
            } 
        }

        // wait to hear from customer that they are done being helped
        if (sig== 1) {
            pthread_mutex_lock(&clerkmutex[clerkID]);
            printf("Clerk %d waiting for %d to finish\n", clerkID, helpingcust);
            pthread_cond_wait(&clerkcond[clerkID], &clerkmutex[clerkID]);
            pthread_mutex_unlock(&clerkmutex[clerkID]);
            printf("Clerk %d finished helping %d\n", clerkID, helpingcust);
        }
    }
    pthread_exit(NULL);
    return NULL;
    
}

void* customer(void* arg) {
    
    Customer* data = (Customer*) arg;
    int qid = data->class;
    

    usleep(data->arrivet * 100000); // Convert to microseconds
    fprintf(stdout, "A customer arrives: customer ID %2d. \n", data->id);

    // customer gets in line
    pthread_mutex_lock(&qmutex[qid]);
    if (head[qid] == NULL) {
        head[qid] = newNode(head[qid], data->id, data->class, data->svct, data->arrivet);
    } else {
        newNode(head[qid], data->id, data->class, data->svct, data->arrivet);
    }
    qlen[qid]++;
    pthread_mutex_unlock(&qmutex[qid]);

    double starttime = getCurrentSimulationTime();

    fprintf(stdout, "Customer %d enters a queue: the queue ID %1d, and length of the queue %2d. \n", data->id, data->class, qlen[data->class]);

    pthread_mutex_lock(&qmutex[qid]);
    // wait until I am at the head of the line and we are chosen by a clerk
    while (head[qid]->id != data->id || custstat[data->id]==-1){
        pthread_cond_wait(&qcond[qid], &qmutex[qid]);
    }

    // I am chosen so I leave the line
    head[qid] = deleteNode(head[qid],data->id);
    qlen[qid]--;
    int clerk = custstat[data->id]; // clerk n is helping 
    pthread_mutex_unlock(&qmutex[qid]);


    usleep(10); // Add a usleep here to make sure that all the other waiting threads have already got back to call pthread_cond_wait. 10 us will not harm your simulation time.
    
    fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", starttime, data->id, clerk);
    usleep(data->svct * 100000);
    double endtime = getCurrentSimulationTime();
    fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", endtime, data->id, clerk);
    
    
    if (qid==BUSINESS){
        businesstime+=endtime-starttime;
        businesstotal++;
    }else{
        economytime+=endtime-starttime;
        economytotal++;
    }

    finished++;
    pthread_cond_signal(&clerkcond[clerk]);
    pthread_exit(NULL);
    return NULL;
}



int main(int argc, char *argv[]) {
    //gettimeofday(&start_time, NULL);
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Initalize all variables
    Customer* customers = readCustomers(argv[1], &ncust);
    if (!customers) {
        return 1;
    }
    int clerkinfo[NCLERKS];
    for (int i=0;i<NCLERKS;i++){
        clerkinfo[i] = i;
    }
    pthread_mutex_init(&start_time_mtex, NULL);
    for (int i = 0; i < NQUEUE; i++) {
        pthread_mutex_init(&qmutex[i], NULL);
        pthread_cond_init(&qcond[i], NULL);
        head[i] = NULL; // Initialize head for each queue
        qlen[i] = 0;    // Initialize queue lengths
    }
    for (int i =0; i <= ncust; i++){
        custstat[i] = IDLE;
    }
    for (int i=0;i<NCLERKS;i++){
        pthread_mutex_init(&clerkmutex[i], NULL);
        pthread_cond_init(&clerkcond[i], NULL);
    }


    // Create customer threads
    custT = malloc(ncust * sizeof(pthread_t));
    if (!custT) {
        perror("Failed to allocate memory for customer threads");
        free(customers);
        return 1;
    }
    for (int i = 0; i < ncust; i++) {
        if (pthread_create(&custT[i], NULL, customer, (void*)&customers[i]) != 0) {
            fprintf(stderr, "Error creating thread for customer ID %d\n", customers[i].id);
        }
    }

    // Create clerk threads
    clerkT = malloc(NCLERKS * sizeof(pthread_t));
    if (!clerkT) {
        perror("Failed to allocate memory for customer threads");
        free(customers);
        return 1;
    }
    for (int i = 0; i < NCLERKS; i++) {
        if (pthread_create(&clerkT[i], NULL, clerk, (void*)&clerkinfo[i]) != 0) {
            fprintf(stderr, "Error creating thread for customer ID %d\n", i);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < ncust; i++) {
        pthread_join(custT[i], NULL);
    }
    for (int i = 0; i < NCLERKS; i++) {
        pthread_join(clerkT[i], NULL);
    }

    // Clean up
    free(custT);
    free(clerkT);
    free(customers);
    pthread_mutex_destroy(&start_time_mtex);
    for (int i = 0; i < NQUEUE; i++) {
        pthread_mutex_destroy(&qmutex[i]);
        pthread_cond_destroy(&qcond[i]);
    }
    for (int i=0;i<NCLERKS;i++){
        pthread_mutex_destroy(&clerkmutex[i]);
        pthread_cond_destroy(&clerkcond[i]);
    }

    // Statistics
    fprintf(stdout, "The average waiting time for all customers in the system is: %.2f seconds. \n", (economytime+businesstime)/ncust);
    if (businesstotal>0) fprintf(stdout, "The average waiting time for all business-class customers is: %.2f seconds. \n", businesstime/businesstotal);
    if (economytotal>0) fprintf(stdout, "The average waiting time for all economy-class customers is: %.2f seconds. \n", economytime/economytotal);
    printf("Handled %d/%d customer complaints\n", finished, ncust);

    return 0;
}
