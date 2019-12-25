#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

int clientCount = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct client{
	int index;
	int sockID;
	struct sockaddr_in clientAddr;
	int len;
    int curRoom;
};


struct client Client[300];
pthread_t thread[1024];


int getClientId(int sock){
    for(int i = 0; i < clientCount; i++){
        if(sock == Client[i].sockID)
            return Client[i].index;
    }
        return -1;
}


void * doNetworking(void * ClientDetail){

	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;

	printf("Client %d connected.\n",index + 1);

	while(1){

		char data[1024];
		int read = recv(clientSocket,data,1024,0);
		data[read] = '\0';

		char output[1024];

		if(strcmp(data,"LIST") == 0){

			int l = 0;

			for(int i = 0 ; i < clientCount ; i ++){

				if(i != index)
					l += snprintf(output + l,1024,"Client %d is at socket %d\n",i + 1,Client[i].sockID);

			}

			send(clientSocket,output,1024,0);
			continue;

		}
		if(strcmp(data,"SEND") == 0){

            read = recv(clientSocket,data,1024,0);
			data[read] = '\0';

            char* ok = "msg sent";
            int authorId = getClientId(clientSocket);
            for(int i = 0; i < clientCount; i++){
                if(Client[i].index != authorId && Client[authorId].curRoom == Client[i].curRoom)
                    send(Client[i].sockID,data,1024,0);
            }
            send(Client[authorId].sockID,ok,1024,0);
            
            
        }		
                  
        if(strcmp(data,"CR") == 0){

			read = recv(clientSocket,data,1024,0);
			data[read] = '\0';

			char* answ = "Changed room";
            int id = getClientId(clientSocket);
            if(Client[id].curRoom == 1)
                Client[id].curRoom = 2;
            else Client[id].curRoom = 1;
            send(clientSocket, answ, 1024, 0);
		}
    
    }
    return NULL;
}

int main(){
	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;


	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9003);
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);


	if(bind(serverSocket,(struct sockaddr *) &serverAddr , sizeof(serverAddr)) == -1) return 0;

	if(listen(serverSocket,1024) == -1) return 0;

	printf("Server started listenting on port 9003 ...........\n");

	while(1){

		Client[clientCount].sockID = accept(serverSocket, (struct sockaddr*) &Client[clientCount].clientAddr, &Client[clientCount].len);
		Client[clientCount].index = clientCount;
        Client[clientCount].curRoom = 1;

pthread_create(&thread[clientCount], NULL, doNetworking, (void *) &Client[clientCount]);

		clientCount ++;

	}

	for(int i = 0 ; i < clientCount ; i ++)
		pthread_join(thread[i],NULL);

}
