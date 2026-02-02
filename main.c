#include <pthread.h>
#include <memory.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "common.h"

//Global Vars
char** stringArray;

void *serverTask(void *args){
    //Do server-y things here
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

    return 0;

}