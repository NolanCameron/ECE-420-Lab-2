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


//Global Vars
char** stringArray;
pthread_rwlock_t** rwlockArray;
int numOfStringsGlobal;



void *serverTask(void *args){
    int clientFileDescriptor = (int)(long)args;
    char msg[COM_BUFF_SIZE];
    
    ClientRequest request;
    //add exception handling
    int total = 0;
    while (total < COM_BUFF_SIZE) {
        int r = read(clientFileDescriptor, msg + total, COM_BUFF_SIZE - total);
        if (r <= 0) {
            close(clientFileDescriptor);
            return NULL;
        }
        total += r;
    }

    printf("Reading from client...\n");
    msg[COM_BUFF_SIZE-1] = '\0';
    ParseMsg(msg,&request);
    if (request.pos < 0 || request.pos >= numOfStringsGlobal) {
        close(clientFileDescriptor);
        return NULL;
    }

    printf("Recieved: %d %d %s\n",request.pos,request.is_read,request.msg);
    if (request.is_read){
        pthread_rwlock_rdlock(rwlockArray[request.pos]);
        getContent(msg, request.pos, stringArray);
        pthread_rwlock_unlock(rwlockArray[request.pos]);
        write(clientFileDescriptor,msg,COM_BUFF_SIZE);
    } else {

        pthread_rwlock_wrlock(rwlockArray[request.pos]);
        setContent(request.msg, request.pos, stringArray);
        getContent(msg, request.pos, stringArray);
        pthread_rwlock_unlock(rwlockArray[request.pos]);
        write(clientFileDescriptor,msg,COM_BUFF_SIZE);
    }



    close(clientFileDescriptor);

    return NULL;
    
}

int main(int argc, char* argv[]){
       
    int numOfStrings = strtol(argv[1], NULL, 10);
    numOfStringsGlobal = numOfStrings;
    rwlockArray = (pthread_rwlock_t**) malloc(numOfStrings * sizeof(pthread_rwlock_t*));
    for (int i = 0; i < numOfStrings; i++){
        rwlockArray[i] = (pthread_rwlock_t*) malloc(sizeof(pthread_rwlock_t));
        pthread_rwlock_init(rwlockArray[i], NULL);
    }
    char *serverIP = argv[2];
    int serverPort = strtol(argv[3], NULL, 10);

    struct sockaddr_in sockAddr;
    int serverFileDescriptor = socket(AF_INET,SOCK_STREAM,0);
    int clientFileDescriptor;

    sockAddr.sin_addr.s_addr=inet_addr(serverIP);
    sockAddr.sin_port=htons(serverPort);
    sockAddr.sin_family=AF_INET;

    pthread_t* threadHandles = malloc(COM_NUM_REQUEST*sizeof(pthread_t));

    stringArray = (char**) malloc(numOfStrings * sizeof(char*));
    for (int i = 0; i < numOfStrings; i ++){
        stringArray[i] = (char*) malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(stringArray[i], "String %d: the initial value", i);
    }


    //Server Loop
    if(bind(serverFileDescriptor, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) >= 0){
        printf("socket has been created\n");
        listen(serverFileDescriptor, COM_NUM_REQUEST*2); //I don't know what to do with this magic number
        
        for(int i = 0; i < COM_NUM_REQUEST; i++){
        
            clientFileDescriptor = accept(serverFileDescriptor, NULL, NULL);
            printf("Connected to client %d\n",clientFileDescriptor);
            pthread_create(&threadHandles[i], NULL, serverTask, (void *)(long)clientFileDescriptor);
        }

        for(int i = 0; i < COM_NUM_REQUEST; i++)
            pthread_join(threadHandles[i], NULL);

        close(serverFileDescriptor);
    }
    else{
        printf("socket creation failed\n");
    }

    //Cleanup
    free(threadHandles);
    for (int i = 0; i < numOfStrings; i ++)
        free(stringArray[i]);
    free(stringArray);
    for (int i = 0; i < numOfStrings; i++) {
        pthread_rwlock_destroy(rwlockArray[i]);
        free(rwlockArray[i]);
    }
    free(rwlockArray);


    return 0;

}