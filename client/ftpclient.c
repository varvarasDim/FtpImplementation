#include "transfer_file_client.h"
#include <netdb.h>
#include <arpa/inet.h>
#define h_addr h_addr_list[0]
int PORT=10011;

int main(int argc,int **argv)
{
	int portmax;
	int portserv;
	int sockfd;	//The socket for communication
	struct sockaddr_in servaddr;	//struck sockaddr with features od socket
	char communBuffer[10]={' '};   //buffer of communication
	int cnt;		//control integer
	int connectRtrn=0;	//control integer for bind and connect
	setbuf(stdout,NULL);	
	struct clientarg ca;	//Structure where there are saved the arguments the user provides when executing ftpclient
	struct hostent *host;	//struct hostent
	portmax=PORT+9;
	portserv=PORT-1;

	sockfd=socket(AF_INET,SOCK_STREAM,0);	//Creation of socket

	bzero(&servaddr,sizeof(servaddr));	//Zeroing of the features of sockaddr_in
	servaddr.sin_family=AF_INET;		//protocol IPv4
	servaddr.sin_port=htons(PORT);		//port of client
	if (!(argv[1]))			//Simple check for arguments
	{
		printf("Wrong structure of the instruction:ftpclient <serverbyname>\n");
		exit(1);
	}
	connectRtrn=bind(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr));	//bind of socket
	while (connectRtrn<0)		//Check if bind completed successfully
	{
		PORT++;
		printf("PORT: %d\n",PORT);
		if (PORT==portmax)	//Or else try to bind at ports until port 10020
		{
			printf("Error at bind\n");
			exit(0);
		}
		servaddr.sin_port=htons(PORT);	
		connectRtrn=bind(sockfd,(struct sockaddr *) &servaddr,sizeof(servaddr)); //try new bind
	}
	printf("\nYou are using port %d for the communication\n",ntohs(servaddr.sin_port));	//show message

	servaddr.sin_port=htons(portserv);	//server port

	if ((host=gethostbyname((char *)argv[1]))==NULL)	//resolve arguments user provided
	{
		herror("gethostbyname");
		exit(1);
	}

	inet_pton(AF_INET,inet_ntoa(*((struct in_addr *)host->h_addr)),&servaddr.sin_addr); //save at struct servaddr
	//printf("\nIP=%s\n",inet_ntoa(*((struct in_addr *)host->h_addr)));	
	//inet_pton(AF_INET,argv[1],&servaddr.sin_addr);

	connectRtrn=connect(sockfd,(struct sockaddr *) &servaddr, sizeof(servaddr)); //connect to server
	if (connectRtrn<0)	//Check if connect completed successfully
	{
		printf("Cannot connect with the FTP server\n");
		exit(0);
	}
	printf("\nType put <filename> <filesize> to upload a file,get <filename> to download a file , ls to see the uploaded files or anything else to exit.\n");	//Μήνυμα οθόνης
	printf("ftpclient:");
	ca=userSelection(ca);	//Function for the choice of user
	switch (ca.argchoice)
	{
	case 1:
		strcpy(communBuffer,"ls_choice");
		cnt=write(sockfd,communBuffer,sizeof(communBuffer));	//user selected ls
		if (cnt<0)
		{
			printf("Error at ls\n");
		}
		instructionLS(sockfd);
		break;
	case 2:
		strcpy(communBuffer,"getchoice");
		cnt=write(sockfd,communBuffer,sizeof(communBuffer));   //user selected get
		if (cnt<0)
		{
			printf("Error at get\n");
		}
		instructionGET(ca,sockfd);
		break;
	case 3:
		strcpy(communBuffer,"putchoice");
		cnt=write(sockfd,communBuffer,sizeof(communBuffer));	//user selected put
		if (cnt<0)
		{
			printf("Error at put\n");
		}
		instructionPUT(ca,sockfd);
		break;
	default:
		printf("Wrong arguments...\n");	//Wront user arguments
		close(sockfd);
		exit(0);
		break;
	}
	close(sockfd);		//Close socket
	exit(0);
	
}
