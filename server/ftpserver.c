#include "transfer_file_server.h"
#include <arpa/inet.h>

int main(int argc,char **argv)
{
	int listenfd,connfd;	//The two sockets of server
	int cnt;		//control integer
	pid_t childpid;		//child id 
	char communBuffer[10]={' '};	//communication buffer 
	char hostname[HOSTNAME];	//The hostname of server
	char connIP[IP_LENGTH];	//ip of server
	int portnumber=0;	//port for server
	int tmpCInstruction=0;	//integer to control the return of the communication port
	socklen_t clilen;
	struct sockaddr_in cliaddr,servaddr;
	signal( SIGCHLD, sig_chld ); /* avoid "zombie" process creation */
	setbuf(stdout,NULL);

	tmpCInstruction=checkInstruction(argc,argv);	//Check if the instructon had the correct structure
	if (tmpCInstruction==0)
	{
		;
	}
	else
		SERVER_PORT=tmpCInstruction;

	listenfd = socket(AF_INET,SOCK_STREAM,0);	//Create the socket where server listens

	bzero( &servaddr , sizeof(servaddr) );	//Zero the fields of servaddr
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERVER_PORT);	//Initialize the structure servaddr
	gethostname(hostname,HOSTNAME);
	FILE *fp;
	printTime();		//Write at logfile the date
	if ((fp=fopen("logfile","a+"))==NULL)
	{
		printf("\nCannot open logfile\n");		//Write at logfile the following message
	}
	else
	{
		fprintf(fp," Starting FTPserver on host:%s,port:%d\n",hostname,ntohs(servaddr.sin_port));
		fclose(fp);
	}
	printf(" Starting FTPserver on host:%s,port:%d\n",hostname,ntohs(servaddr.sin_port));
	bind(listenfd, (struct sockaddr *) &servaddr , sizeof(struct sockaddr) );	//bind of socket
	listen(listenfd, LISTENQ);

	printTime();//Write at logfile the date
	if ((fp=fopen("logfile","a+"))==NULL)
	{
		printf("\nCannot open logfile\n");		
	}
	else
	{
		fprintf(fp," Server intialization completed - waiting for connections...\n");
		fclose(fp);
	}



	while ( 1 )	//infinite loop
	{	
		clilen=sizeof(cliaddr);
		if ( ( connfd=accept(listenfd,(struct sockaddr *)&cliaddr,&clilen) )<0)	// accept when a new connection
			if (errno=EINTR)
				continue;
			else
				err_sys("accept error");
		
		strcpy(connIP,(char *)inet_ntoa(cliaddr.sin_addr));	//Save at connIP the IP address of the client
		portnumber=ntohs(cliaddr.sin_port);	//Save at portnumber at which port connected client
		printTime();	//Write at logfile the date
		if ((fp=fopen("logfile","a+"))==NULL)
		{
			printf("\nCannot open logfile\n");	
		}
		else
		{
			fprintf(fp," Connection accepted from %s port:%d\n",connIP,portnumber);
			fclose(fp);
		}
		if (system("ls -1 > filelist\n")<0)	//Create the file filelist where it contains the files to download
		{
			perror("error");
		}
		childpid=fork();	//Create child
		if ( childpid==0 )	//Only first child gets in here
		{
			close(listenfd);	//Close socket wht server listens
			//printTime();
			cnt=read(connfd,communBuffer,sizeof(communBuffer));	//First read of what the client wants
			if (cnt<0)
			{
				printf("\nError at first read\n");
				userDisconnected(connIP,portnumber);
				exit(0);
			}
			if (strcmp(communBuffer,"ls_choice")==0)	//ls selection
			{
				instruction_LS_serv(connfd,connIP);				

			}
			else if (strcmp(communBuffer,"getchoice")==0)	//get selection
			{
				instruction_GET_serv(connfd,connIP);
			}
			else if (strcmp(communBuffer,"putchoice")==0)	//put selection
			{
				instruction_PUT_serv(connfd,connIP);
			}
			else
			{
				printf("\nWrong instruction from the user\n");		//Wrong selection
				
			}
			userDisconnected(connIP,portnumber);	//disconnect
			portnumber=0;
			zeroBuffer(communBuffer,10);		//zero communbuffer and portnumber 
			close(connfd);		//close socket connfd
			exit(0);		//exit child

		}
		close(connfd);		//close  connfd of father
	}
	close(listenfd);	//close listenfd of father
	return (1);
}


