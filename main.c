#include <pthread.h>
#include <memory.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include <errno.h>
#include <stdlib.h>


#include "common.h"
#include <unistd.h>
#include <sys/time.h>


//Global Vars
char** stringArray;
pthread_rwlock_t** rwlockArray;
int numOfStringsGlobal;
double* times;
struct timeval* startTimes = NULL;
struct timeval* endTime = NULL;

typedef struct {
    int clientFileDescriptor;
    int requestID;
} Threadargs;

void *serverTask(void *arg){
    Threadargs *args = (Threadargs *)arg;
    int clientFd = args->clientFileDescriptor;
    int requestID = args->requestID;
    free(arg);
    char msg[COM_BUFF_SIZE];

    
    ClientRequest request;
    //add exception handling
    int total = 0;
    while (total < COM_BUFF_SIZE) {
        int r = read(clientFd, msg + total, COM_BUFF_SIZE - total);
        if (r <= 0) {
            close(clientFd);
            return NULL;
        }
        total += r;
    }

    printf("Reading from client...\n");
    msg[COM_BUFF_SIZE-1] = '\0';
    ParseMsg(msg,&request);
    if (request.pos < 0 || request.pos >= numOfStringsGlobal) {
        close(clientFd);
        return NULL;
    }

    printf("Recieved: %d %d %s\n",request.pos,request.is_read,request.msg);
    if (request.is_read){
        pthread_rwlock_rdlock(rwlockArray[request.pos]);
        getContent(msg, request.pos, stringArray);
        pthread_rwlock_unlock(rwlockArray[request.pos]);
        write(clientFd,msg,COM_BUFF_SIZE);
    } else {

        pthread_rwlock_wrlock(rwlockArray[request.pos]);
        setContent(request.msg, request.pos, stringArray);
        getContent(msg, request.pos, stringArray);
        pthread_rwlock_unlock(rwlockArray[request.pos]);
        write(clientFd,msg,COM_BUFF_SIZE);
    }
    gettimeofday(&endTime[requestID], NULL);
    times[requestID] =
    (endTime[requestID].tv_sec - startTimes[requestID].tv_sec) * 1e6 +
    (endTime[requestID].tv_usec - startTimes[requestID].tv_usec);
    printf("Request %d completed in %lf microseconds\n", requestID, times[requestID]);
    close(clientFd);

    return NULL;
    
}

int main(int argc, char* argv[]){
       
    int numOfStrings = strtol(argv[1], NULL, 10);
    numOfStringsGlobal = numOfStrings;
    rwlockArray = (pthread_rwlock_t**) malloc(numOfStrings * sizeof(pthread_rwlock_t*));

    times = (double*) malloc(COM_NUM_REQUEST * sizeof(double));
    char *serverIP = argv[2];
    int serverPort = strtol(argv[3], NULL, 10);

    struct sockaddr_in sockAddr;
    int serverFileDescriptor = socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;

    sockAddr.sin_addr.s_addr=inet_addr(serverIP);
    sockAddr.sin_port=htons(serverPort);
    sockAddr.sin_family=AF_INET;

    pthread_t* threadHandles = malloc(COM_NUM_REQUEST*sizeof(pthread_t));
    startTimes = malloc(COM_NUM_REQUEST * sizeof(struct timeval));
    endTime = malloc(COM_NUM_REQUEST * sizeof(struct timeval)); 

    stringArray = (char**) malloc(numOfStrings * sizeof(char*));
    for (int i = 0; i < numOfStrings; i ++){
        stringArray[i] = (char*) malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(stringArray[i], "String %d: the initial value", i);
        rwlockArray[i] = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t));
        pthread_rwlock_init(rwlockArray[i], NULL);
    }


    //Server Loop
    if(bind(serverFileDescriptor, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) >= 0){
        printf("socket has been created\n");
        listen(serverFileDescriptor, COM_NUM_REQUEST*2); //I don't know what to do with this magic number
        
        for(int i = 0; i < COM_NUM_REQUEST; i++){
                    
            clientFileDescriptor = accept(serverFileDescriptor, NULL, NULL);            
            printf("Connected to client %d\n",clientFileDescriptor);
            Threadargs* args = malloc(sizeof(Threadargs)); 
            args->clientFileDescriptor = clientFileDescriptor;
            args->requestID = i;
            gettimeofday(&startTimes[i], NULL);            
            pthread_create(&threadHandles[i], NULL, serverTask, args);
        }

        for(int i = 0; i < COM_NUM_REQUEST; i++)
            pthread_join(threadHandles[i], NULL);

        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }
    saveTimes(times, COM_NUM_REQUEST);

    //Cleanup
    for (int i = 0; i < numOfStrings; i++) {
        pthread_rwlock_destroy(rwlockArray[i]);
        free(rwlockArray[i]);
        free(stringArray[i]);
    }
    free(startTimes);
    free(endTime);
    free(times);
    free(rwlockArray);
    free(threadHandles);
    free(stringArray);


    return 0;

}