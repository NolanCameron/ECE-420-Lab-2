#define _POSIX_C_SOURCE 200112L //fixes an error where rwlocks are undefined
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

//Global Vars
char** stringArray;
pthread_rwlock_t* locks;

void *serverTask(void *args){
    int clientFileDescriptor = (int)(long)args;
    char msg[COM_BUFF_SIZE];
    
    ClientRequest request;
    //add exception handling
    read(clientFileDescriptor,msg,12);
    printf("Reading from client: %d\n",clientFileDescriptor);
    ParseMsg(msg,&request);
    printf("Recieved: %d %d %s\n",request.pos,request.is_read,request.msg);

    if(request.is_read){
        //reading
        printf("reading");
        pthread_rwlock_rdlock(&locks[request.pos]);
        printf("before: %s",stringArray[request.pos]);//remove after testing
        getContent(request.msg,request.pos,&stringArray[request.pos]);
        printf("after: %s",stringArray[request.pos]);//remove after testing
        pthread_rwlock_unlock(&locks[request.pos]);
    }else{
        //writing
        printf("riting");
        pthread_rwlock_wrlock(&locks[request.pos]);
        //set content
        pthread_rwlock_unlock(&locks[request.pos]);

    }

    close(clientFileDescriptor);

    return NULL;
    
}

int main(int argc, char* argv[]){

    int numOfStrings = strtol(argv[1], NULL, 10);
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
    locks = malloc(numOfStrings * sizeof(pthread_rwlock_t));

    for (int i = 0; i < numOfStrings; i ++){
        stringArray[i] = (char*) malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(stringArray[i], "String %d: the initial value", i);
        pthread_rwlock_init(&locks[i],NULL);

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

    return 0;

}