#ifndef __TRANSFER_FILE_CLIENT__
#define __TRANSFER_FILE_CLIENT__

#include <stdio.h>		//libraries
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

#define MAXLINE 16
#define LISTENQ 10


struct clientarg{	//Struct that is used for the analysis of the user's arguments

	char buffer_arg0[20];	//first arg
	char buffer_arg1[20];	//second arg
	char buffer_arg2[20];	//third arg

	int argcount;//arguments number
	int argchoice; //if 1 then LS if 2 then GET if 3 then PUT
} clientarg;

//////////////////////////////////////////Functions//////////////////////////////////////////////////////////////////

ssize_t readnB(int fd, void *vptr, size_t n)
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


ssize_t writenB(int fd, const void *vptr, size_t n)
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
        int i=0;
	int temporary=0;
	FILE *fp;
	if ((fp=fopen(filename,"rb"))!=NULL)
		return -3;
	
        for (i=0;i<3073;i++)
        {
                buffer[i]=0;
        }

// begin receiving file
	  if (filesize >= 3072)
            bytes_read_this_time=readnB(tcp_socket,buffer,3072);
	  else 
	    bytes_read_this_time=readnB(tcp_socket,buffer,filesize);

		temporary=(int )bytes_read_this_time;
		if (temporary<=0)
		{
			return -1;				
		}
		
	//printf("\nBRTT:%d\n",(int *)bytes_read_this_time);

// It has read the first STRING that was sent

        file_desc=creat(filename,0644);
        if (file_desc < 0)
        {
                close(tcp_socket);
                printf("\nCan not create filename %s\n",filename);
                return -1;             // Could not create filename
        }
        printf("new file created\n");
        //
        // Writes the first bytes that from the file that was sent
        //
        size_left_to_download-=bytes_read_this_time;
        if(bytes_read_this_time > 0)
        {
                bytes_written_to_file = writenB(file_desc,buffer,bytes_read_this_time);
                if(bytes_written_to_file < bytes_read_this_time)
                {
                         close(tcp_socket);
                         printf("\nCannot save file %s\n",filename);
                         return -3;  // Cannot save file, probably because of insufficient disk space
                }
        }


        printf("\n %3f%% downloaded ",100*(1-((double)size_left_to_download/(double)filesize)));

        while (size_left_to_download > 0 )  // while loop in order to get the complete file
        {
                // read the next bytes that were received

                if (size_left_to_download>= 3072)
				bytes_read_this_time=readnB(tcp_socket,buffer,3072);
		    else
			bytes_read_this_time=readnB(tcp_socket,buffer,size_left_to_download);

			temporary=(int )bytes_read_this_time;
			if (temporary<=0)
			{
				return -1;				
			}
                // writes the bytes that were read
                size_left_to_download-=bytes_read_this_time;
                if(bytes_read_this_time > 0)
                {
                        bytes_written_to_file = writenB(file_desc,buffer,bytes_read_this_time);
                        if(bytes_written_to_file < bytes_read_this_time)
                        {
                                close(tcp_socket);
                                printf("\nCannot save file %s\n",filename);
                                return -3;  // Cannot save file, probably because of insufficient disk space
                        }
                }
                printf("\n");
                printf("%3f%% downloaded ",100*(1-((double)size_left_to_download/(double)filesize)));
        }
   
        close(file_desc);   // Closes file
        close(tcp_socket);
        printf ("\n File %s received successfully\n",filename);
        return 0;

}

int findSizeOfFile(char namebuffer[])	//Finds size of file by its name
{
	FILE *fp;
	int l=0;
	char nb[20]={' '};	//name of file locally
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
		fclose(fp);	//Opens binary file and counts its size
	}
	return (l-1);
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
        nbytes = readnB(fd,buffer,3072);

        while (nbytes > 0)
        {
                // read data and sent them  via tcp_socket
                if (writenB(tcp_socket,buffer,nbytes)<0)
		{
			return -1;
		}

                // read next section
                nbytes = readnB(fd,buffer,3072);

        }
        return 0;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void mygetline(char s[],int k)
{			//Function that gets text from cli and saves it to a buffer
			//Arguments are the buffer and the size of the buffer

	int c,i;
	for (i=0;(i<k-1) && ((c=getchar())!='\n')  ; ++i )
	{
		s[i]=c;
	}
	s[i]='\0';
}
void wrongSelection()	//Simple wrong selectiin print
{
	printf("\nWrong arguments...\n");
	printf("Type put <filename> <filesize> to upload a file,get <filename> to download a file , ls to see the uploaded files or anything else to exit.\n");
	exit(0);
}

struct clientarg userSelection(struct clientarg cliarg)	//Function to analyse what the user selected
{							//argument is a struct that is being filled and returned after
	int i=0;
	char *arg[4]={0};	//Pointers in char
	char choicebuffer[200];		//The complete user selection
	mygetline(choicebuffer,200);

	arg[0]=strtok(choicebuffer," ");	//Analyse selection and save it to arg[] 
	while (  (arg[i]) && (i<3) )	
	{
		i++;
		arg[i]=strtok('\0'," ");
		if (arg[i]) 
		{
			//printf(" %s ",arg[i]);
		}
	}
	if (arg[0])		//Save the features of the selection in the struct
	{
		strcpy(cliarg.buffer_arg0,arg[0]);
		//printf("%s\n",cliarg.buffer_arg0);
	}
	if (arg[1])
	{
		strcpy(cliarg.buffer_arg1,arg[1]);
		//printf("%s\n",cliarg.buffer_arg1); 
	}
	if (arg[2])
	{
		strcpy(cliarg.buffer_arg2,arg[2]);
		//printf("%s\n",cliarg.buffer_arg2);
	}
	
	cliarg.argcount=i;	//Save the number of names at the struct 
	if (cliarg.argcount==0)	//if nothing
	{
		wrongSelection();
	}
	else if (cliarg.argcount==1)	//If only one argument the it must be "ls" else wrong
	{
		if (strcmp(arg[0],"ls")!=0)
		{
			wrongSelection();
		}
		else
		{
			//printf("okls");
			cliarg.argchoice=1;	//Pass at struct what the user selected
		}
	}
	else if(cliarg.argcount==2)	//If two arguments then first one must be "get" else wrong
	{
		if (strcmp(arg[0],"get")!=0)
		{
			wrongSelection();
		}
		else
		{	
			//printf("okget");
			cliarg.argchoice=2;	
		}
	}
	else if(cliarg.argcount==3)	//If three arguments then first on must be "put" else wrong
	{
		if (strcmp(arg[0],"put")!=0)
		{
			wrongSelection();
		}
		else
		{
			//printf("okput");
			cliarg.argchoice=3;
		}
	}
	else		//If more than three  show error
	{
		wrongSelection();
	}
	return cliarg;	
}
void instructionLS(int socket)	//LS selection ,the argument is the socket
{
	int cnt;	//control integer
	char ch;	//char to send data from server
	char communBuffer[10]={' '};	//communication buffer 
	cnt=read(socket,communBuffer,sizeof(communBuffer));	//read what server sent
	if (cnt<0)
	{
		printf("Error inside instructionLS()\n");
		exit(0);
	}
	if (strcmp(communBuffer,"ls_ok____")==0)	//if ls_ok then continue
	{
		while ( cnt=read( socket, &ch, sizeof( char ) ) > 0 )	//read list and show on console
			putchar( ch );
	}
	else	//Or else show error and exit
	{
		printf("Error inside instructionLS()\n");
		exit(0);	
	
	}



}

void instructionGET(struct clientarg ca,int socket)	//Selection GET, arguments are the socket and the struct cliengarg
{
	struct clientarg clia=ca;	//save struct locally
	int cnt;	// control integer
	int check;	//control integer for the return of function store_a_file
	int filesize=0;	//file size
	setbuf(stdout,NULL);
	char communBuffer[10]={' '};	//communication buffer

	cnt=read(socket,communBuffer,sizeof(communBuffer));	//Read what server sents
	if (cnt<0)
	{
		printf("Error inside instructionGET()\n");
		exit(0);
	}
	if (strcmp(communBuffer,"getok____")==0)	//if server sent getok then everything ok
	{
		cnt=write(socket,clia.buffer_arg1,sizeof(clia.buffer_arg1));//Sends to server the name of the file
		if (cnt<0)
		{
			printf("Error at instructionGET()\n");
			exit(0);
		}
		cnt=read(socket,&filesize,sizeof(int));//Reads the size of the file to use in  store_a_file
		if (cnt<0)
		{
			printf("Error inside instructionGET()\n");
			exit(0);
		}
		check=store_a_file(clia.buffer_arg1,filesize,socket);//Receives file and stores it to the folder
		switch (check)
		{
		case 0:	
			printf("File downloaded\n");//According to what is being returned by store_a_file he corresponding message is being printed
			break;
		case -1:
			printf("Create failed\n");
			break;
		case -2:
			printf("Unknown failure in read\n");
			break;
		case -3:
			printf("Problem at write\n");
			break;
		default:
			printf("UNKNOWN PROBLEM\n");
			break;
		}
	}
	else			
	{	
		printf("Error at instructionGET\n");
	}
	
	
}
void instructionPUT(struct clientarg ca,int socket)	//selectino put
{							// arguments are the socket and the struct clientarg
	int cnt;	//control integer
	char communBuffer[10]={' '};     //communication buffer
	int computedFilesize=0;
	
	struct clientarg clia=ca;	//Save structure clientarg locally
	int i;
	int check;		//control integer for the return of store_a_file
	setbuf(stdout,NULL);
	computedFilesize=findSizeOfFile(clia.buffer_arg1);
	
	cnt=read(socket,communBuffer,sizeof(communBuffer));	//Read what server has sent back
	if (cnt<0)
	{
		printf("Error inside instructionPUT()\n");
		exit(0);
	}
	if (strcmp(communBuffer,"putok____")==0)	//if it sent putok then go on
	{
		
		cnt=write(socket,clia.buffer_arg2,sizeof(clia.buffer_arg2));
		if (cnt<0)			//It sents the filesize that will be uploaded
		{
			printf("Error during sending the size of the file\n");
			exit(0);
		}
		cnt=write(socket,clia.buffer_arg1,sizeof(clia.buffer_arg1));
		if (cnt<0)			//It sents the name of the file that wants to upload
		{
			printf("Error during sending the name of the file\n");
			exit(0);
		}
		if (!(atoi(clia.buffer_arg2)))
		{
			printf("Wrong arguments...\n");
			close(socket);
			exit(0);
		}
		if ((computedFilesize>=atoi(clia.buffer_arg2)))
		{
			check=send_a_file(clia.buffer_arg1,socket);	//execute function send_a_file
			if (check==0)	//Check the return of send_a_file
			{
				printf("File send\n");
			}
			else
			{
				printf("Error,file not send\n");
			}
		}
		else
		{
			printf("Wrong size\n");
			close(socket);
			exit(0);
		}
		
	}
	else		
	{
		printf("Error inside instructionPUT()\n");
		exit(0);	
	
	}


}
#endif

