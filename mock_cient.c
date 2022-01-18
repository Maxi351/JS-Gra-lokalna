#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>

void * TalkingThread (void *arg){
  int clientSocket = *((int *)arg);
  char message[] = "00";
  int msg;
  printf("am talking to %d \n",clientSocket);  
  for(;;){
        scanf("%d",&msg);
        if(msg!=2 && msg!=1)continue;
        
        message[0]=(char)msg;
        message[1]=(char)1;
        if( send(clientSocket , message , sizeof(message) , 0) < 0)
        {
                perror("Error ");
                printf("Send failed\n");
        }
        else printf("wyslano \n");
  }
  pthread_exit(NULL);
}

void * ListeningThread(void *arg){
  int clientSocket = *((int *)arg);
  char buffer[15];
  int message[15];
  for(;;){
    printf("listening ");
    if(recv(clientSocket, buffer, 15, 0) < 0)
        {
            printf("Receive failed\n");
        }
    //Print the received message
    for(int i=0; i<15 ; i++)
    {
      message[i]=(int)buffer[i];
      printf("%d ",message[i]);
    }
    if(message[0]==0)continue;
    printf(" is the data received \n ");
    


    memset(&buffer, 0, sizeof (buffer));
  }
  pthread_exit(NULL);
}


int main(){

  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addr_size;

  // Create the socket. 
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);

  //Configure settings of the server address
 // Address family is Internet 
  serverAddr.sin_family = AF_INET;

  //Set port number, using htons function 
  serverAddr.sin_port = htons(1100);

 //Set IP address
  serverAddr.sin_addr.s_addr = inet_addr("192.168.127.128");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Connect the socket to the server using the address
    addr_size = sizeof serverAddr;
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
    //strcpy(message,"Hello");
    pthread_t listen_id;
    pthread_t talk_id;    

    if( (pthread_create(&listen_id, NULL, ListeningThread, &clientSocket) )!= 0 )
            printf("Failed to create thread\n");
    if( (pthread_create(&talk_id, NULL, TalkingThread, &clientSocket)) != 0 )
            printf("Failed to create thread\n");    
    pthread_detach(listen_id);
     pthread_detach(talk_id);    


        //Read the message from the server into the buffer
        
    printf("Am waiting\n");
    for(;;);
    printf("Stopped waiting\n");
    //close(clientSocket);
    
  return 0;
}
