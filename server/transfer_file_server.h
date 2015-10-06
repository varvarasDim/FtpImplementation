#ifndef __TRANSFER_FILE_SERVER__
#define __TRANSFER_FILE_SERVER__

#include <stdio.h>	//libraries
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <sys/un.h>		/* for Unix domain sockets */
#include <stdlib.h>
#include <ctype.h>
#include <netinet/in.h>
#include <time.h>

#define MAXLINE 16
#define LISTENQ 10
#define HOSTNAME 50
#define IP_LENGTH 50

int SERVER_PORT=10010;	//port where server runs
void printTime()	//print to logfile the date
{
	FILE *fp;
	struct tm *time_var;
	time_t lt;
	char buffer[25]={' '};		//save date at  ascii format
	if ((fp=fopen("logfile","a+"))==NULL)	//open or create file
	{
		printf("\nCannot open logfile\n");
	}
	else
	{
		lt=time(NULL);
		time_var=localtime(&lt);
		strcpy(buffer,asctime(time_var));
		fprintf(fp,"[_");
		buffer[24]=95;
		fprintf(fp,"%s",buffer);
		fprintf(fp,"] ");
		fclose(fp);
	}

	
}
void userDisconnected(char cIP[IP_LENGTH],int port)	//Function where writes at logfile the disconnection from server
{						//along to ip and port of client
	FILE *fp;
	char connIP[50]={' '};	//client ip
	strcpy(connIP,cIP);
	printTime();
	if ((fp=fopen("logfile","a+"))==NULL)
	{
		printf("\nCannot open logfile\n");
	}
	else
	{
		fprintf(fp," User at (IP,port):(%s,%d) disconnected\n",connIP,port);
		fclose(fp);
	}

}

///////////////////////////////////////////////////functions///////////////////////////////////////////////////////////////////////////

ssize_t                                         /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
        size_t  nleft;
        ssize_t nread;
        char    *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nread = read(fd, ptr, nleft)) < 0) {
                        if (errno == EINTR)
                                nread = 0;              /* and call read() again */
                        else
                                return(-1);
                } else if (nread == 0)
                        break;                          /* EOF */

                nleft -= nread;
                ptr   += nread;
        }
        return(n - nleft);              /* return >= 0 */
}


ssize_t                                         /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
        size_t          nleft;
        ssize_t         nwritten;
        const char      *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (errno == EINTR)
                                nwritten = 0;           /* and call write() again */
                        else
                                return(-1);                     /* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}



// store_a_file is used in order to save a file
// Args: char *filename = name of file
//		 ssize_t filesize = size of file
//		 int tcp_socket   = socket of tcp communication
// RETURNS : 0 = if file was successfully sent
//			   -1 = Can not vreate file that will accept it (creat() failed)
//			   -2 = Unknown problem at read
//			   -3 = there is a problem with writing file
//

int store_a_file (char *filename, ssize_t  filesize, int tcp_socket)
{
        char buffer[3073];   // buffer tou 3KB
        ssize_t bytes_read_this_time=0;
        ssize_t bytes_written_to_file=0;
        ssize_t size_left_to_download=filesize;
        int file_desc;
	int temporary=0;
        int i=0;
	FILE *fp;
	if ((fp=fopen(filename,"rb"))!=NULL)
		return -3;
	
        for (i=0;i<3073;i++)
        {
                buffer[i]=0;
        }

// begin receiving file
	  if (filesize >= 3072)
            bytes_read_this_time=readn(tcp_socket,buffer,3072);
	  else 
	    bytes_read_this_time=readn(tcp_socket,buffer,filesize);
		temporary=(int )bytes_read_this_time;
		if (temporary<=0)
		{
			return -1;				
		}
		

//has read the first string that was sent

        file_desc=creat(filename,0644);
        if (file_desc < 0)
        {
                close(tcp_socket);
                printf("\nCould not create  %s\n",filename);
                return -1;             // Could not create filename
        }
        printf("new file created\n");
        //
        // write the first bytes of the file
        //
        size_left_to_download-=bytes_read_this_time;
        if(bytes_read_this_time > 0)
        {
                bytes_written_to_file = writen(file_desc,buffer,bytes_read_this_time);
                if(bytes_written_to_file < bytes_read_this_time)
                {
                         close(tcp_socket);
                         printf("\ncan not write file %s\n",filename);
                         return -3;  //can not write file insufficient disk space
                }
        }


        printf("\n %3f%% downloaded ",100*(1-((double)size_left_to_download/(double)filesize)));

        while (size_left_to_download > 0 )  //  while loop in order to receive complete file
        {
                // read next byte that were sent

                if (size_left_to_download>= 3072)
				bytes_read_this_time=readn(tcp_socket,buffer,3072);
		    else
			bytes_read_this_time=readn(tcp_socket,buffer,size_left_to_download);
			temporary=(int )bytes_read_this_time;
			if (temporary<=0)
			{
				return -1;				
			}
                // write the byte that was read
                size_left_to_download-=bytes_read_this_time;
                if(bytes_read_this_time > 0)
                {
                        bytes_written_to_file = writen(file_desc,buffer,bytes_read_this_time);
                        if(bytes_written_to_file < bytes_read_this_time)
                        {
                                close(tcp_socket);
                                printf("\ncannot write file %s\n",filename);
                                return -3;  // cannot write file, insufficient disk space
                        }
                }
                printf("\n");
                printf("%3f%% downloaded ",100*(1-((double)size_left_to_download/(double)filesize)));
        }
   
        close(file_desc);   // close file
        close(tcp_socket);
        printf ("\n File %s received successfully \n",filename);
        return 0;

}



// Function that opens the file and reads by per 3KB
// and sends its contents over a socket
int send_a_file(char *filename,int tcp_socket)
{
        char buffer[3073];   // 3KB BUFFER
        int fd;
        int nbytes=0;   // Number of bytes that were read
        int i=0;
        for (i=0;i<3073;i++)
        {
                buffer[i]=0;
        }


        fd = open(filename, O_RDONLY,0);
        if (fd < 0)
        {
                printf("Can not open file %s to read it \n",filename);
                return -1;
        }
        nbytes = readn(fd,buffer,3072);
	//printf("\n\nn%d\n\n",nbytes);
        while (nbytes > 0)
        {
                // read data and sent them  via tcp_socket
                if (writen(tcp_socket,buffer,nbytes)<0)
		{
			return -1;
		}
                //  read next section
                nbytes = readn(fd,buffer,3072);

        }
        return 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void sig_chld( int signo )
{				//use sig_child to kill zombie processes
				
	pid_t pid;
	int stat;
	while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 ) {
		printf( "Child %d terminated.\n", pid );
	}
}
int findSizeOfFile(char namebuffer[])	//Finds size of file by its name
{
	FILE *fp;
	int l=0;
	char nb[20]={' '};	
	char ch;
	strcpy(nb,namebuffer);
	//printf("\nfindSizeOfFile\n");
	if ((fp=fopen(nb,"rb"))==NULL)
	{
		printf("\nCannot open file\n");
	}	
	else
	{
		while (!feof(fp))
		{
			ch=fgetc(fp);
			l++;
		}
		fclose(fp);
	}
	return (l-1);
}
void readFromListFile(int socket)	//reads from filelist
{
	FILE *fp;
	char *p,ch;
	int cnt,rmv;
	int h=0;
	if ((fp=fopen("filelist","a+"))==NULL)	//opens filelist or creates it
	{
		printf("\nCannot open file\n");
	}	
	else
	{
		//printf("\n");			//reads one by one the characters of filelist
		for (;;)			//and sents them to client
		{
			
			ch=fgetc(fp);
			if (ch==EOF)
				break;
			//putchar(ch);
			cnt=write(socket,&ch,sizeof(char));
			if (cnt<0)
			{
				printf("\nError sending the list\n");
			}
		}
		fclose(fp);
	}

}
int checkInstruction(int argc,char *argv[])	//Checks if arguments are correct

	int everythingok=0;	//integer to check if everything ok
	int tmp,i;		//temporary integers
	char argumm2[6]={' '};	//char array to save port
	if (argc==1)	//if nothing is being used then use default port
	{
		SERVER_PORT=10010;
	}
	else if (argc==2)	//if one more argument is being provided the show error message
	{
		printf("\nUsage: ftpserver -p <port>\n");
		exit(0);
	}
	else if (argc==3)	//If 2 more arguments the check if the arguments are the right ones
	{
		strcpy(argumm2,argv[2]);
		if (strcmp("-p",argv[1])==0)	//if second arg is "-p"
		{
			if ((strlen(argv[2])==4)||(strlen(argv[2])==5))	//if the length of the 3rd argument is 4 or 5 chars
			{
				for (i=0;i<strlen(argv[2]);i++)	//then check if it contains numbers
				{
					if ((tmp=isdigit(argumm2[i]))==0)
					{
						everythingok=-1;	//is there a char that is not a number?
					}
				}
				if (everythingok==0)
				{
					everythingok=atoi(argumm2);
				}
			}
			else
			{
				everythingok=-1;	//The length of the argument is not of 4 or 5 chars(1000-99999) (ports:1024-65536)
			}
		}
		else
		{
			everythingok=-1;	//Second arg is not  "-p"
		}
	}
	else
	{
		everythingok=-1;	//If more than 3 aruments
	}
	if (everythingok<0)	//there is a problem
	{
		printf("\nUsage: ftpserver -p <port>\n");
		exit(0);
	}
	return everythingok;


}

void instruction_LS_serv(int socket,char cIP[IP_LENGTH])	//function for ls with args socket and ip of client
{
	int cnt;	//control integer 
	char connIP[50]={' '};	//client ip locally
	FILE *fp;

	printf("\nInstruction LS\n");

	strcpy(connIP,cIP);
	cnt=write(socket,"ls_ok____",sizeof("ls_ok____"));	//sents to client that ls was received successfully
	if (cnt<0)
	{
		printf("\nError at instruction_LS_serv\n");
	}
	printTime();	//Write to logfile of the date
	if ((fp=fopen("logfile","a+"))==NULL)	//writes at logfile what user selected
	{
		printf("\nCannot open logfile\n");
	}
	else
	{
		fprintf(fp," Request from %s :ls\n",connIP);
		fclose(fp);
	}

	readFromListFile(socket);	

}
void instruction_GET_serv(int socket,char cIP[IP_LENGTH])	//Function for get, args are socket and ip client
{
	char namebuffer[20]={' '};	//here is saves the filename
	char connIP[IP_LENGTH]={' '};		//client ip
	FILE *fp;
	int check;	//control integers 
	int cnt;	//	--
	int filesize=0;		//here is being saves the size of the file

	printf("\nInstruction GET\n");


	strcpy(connIP,cIP);
	cnt=write(socket,"getok____",sizeof("getok____"));	//Sent to the client that received get successfully
	if (cnt<0)
	{
		printf("\nError at instruction_GET_serv\n");
	}
	cnt=read(socket,namebuffer,sizeof(namebuffer));		//Reads the name of file that the users wants
	if (cnt<0)
	{	
		printf("\nError during receiving the name of the file\n");
	}
	filesize=findSizeOfFile(namebuffer);		//find filesize of the particular file
	printTime();
	if ((fp=fopen("logfile","a+"))==NULL)	//write selection at logfile
	{
		printf("\nCannot open logfile\n");
	}
	else
	{
		fprintf(fp," Request from %s :get %s\n",connIP,namebuffer);	
		fclose(fp);
	}
	cnt=write(socket,&filesize,sizeof(int));	//Sents to client the size of the file that wants to receive
	if (cnt<0)
	{
		printf("\nError at instruction_GET_serv\n");
	}
	check=send_a_file(namebuffer,socket);		//Sents the file to the user
	if (check==0)		//Checks if sent was sucessful
	{
		printf("\nFile sent\n");
	}
	else
	{
		printf("\nError,file not sent\n");
	}

}

void instruction_PUT_serv(int socket,char cIP[IP_LENGTH])	//function for the put 
{							//args are socket and client ip
	int cnt;		//control integer
	FILE *fp;
	char sizebuffer[20]={' '};	//char array with the file size
	char namebuffer[20]={' '};	//filename
	char connIP[IP_LENGTH]={' '};	//client ip
	int sizebuffer2integer=0;	//buffer to save the filesize in integer format

	printf("\nInstruction PUT\n");

	strcpy(connIP,cIP);
	cnt=write(socket,"putok____",sizeof("putok____"));	//sent pback that put function received sufessfully
	if (cnt<0)
	{
		printf("\nError at instruction_PUT_serv\n");
	}
	int check;
	cnt=read(socket,sizebuffer,sizeof(sizebuffer));	//receive filesize
	if (cnt<0)
	{	
		printf("\nError during receiving the size of the file\n");
	}
	sizebuffer2integer=atoi(sizebuffer);	//cast to an integer
	cnt=read(socket,namebuffer,sizeof(namebuffer));	//read filename
	if (cnt<0)
	{	
		printf("\nError during receiving the name of the file\n");
	}
	printTime();
	if ((fp=fopen("logfile","a+"))==NULL)	//Writes at logfile the clients selection
	{
		printf("\nCannot open logfile\n");
	}
	else
	{
		fprintf(fp," Request from %s :put %s %s\n",connIP,namebuffer,sizebuffer);
		fclose(fp);
	}
	check=store_a_file (namebuffer,sizebuffer2integer,socket);	//Receive file from  client
	switch (check)

	{
	case 0:
		printf("\nFile uploaded\n");		//According to what was returned from store_a_file print the appropriate message
		//writeAtListFile(namebuffer,sizebuffer);	
		break;
	case -1:
		printf("\nCreate failed\n");
		break;
	case -2:
		printf("\nUnknown failure in read\n");
		break;
	case -3:
		printf("\nProblem at write\n");
		break;
	default:
		printf("\nUNKNOWN PROBLEM\n");
		break;
	}


}
void zeroBuffer(char *buf,int size)	
{
	int k=0;
	for (k=0;k<size;k++)
	{
		buf[k]=' ';
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readLogFile()		//Function to read from  logfile				///
{				//NOT USED
	FILE *fp;
	char *p,ch;
	if ((fp=fopen("logfile","r"))==NULL)
	{
		printf("\nCannot open logfile\n");
	}	
	else
	{	
		for (;;)
		{
			
			ch=fgetc(fp);
			if (ch==EOF)
				break;
			putchar(ch);
		}
		fclose(fp);
	}

}													///
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif


