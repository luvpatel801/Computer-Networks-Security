/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <poll.h>

#define BUFSIZE 1000
#define portno 820

/* 
 * error - wrapper for perror
 */
struct sockaddr_in serveraddr;

int Ackwait(int sockfd,int serverlen){
    char buf[BUFSIZE];
    struct pollfd fd;
    int res;
    fd.fd = sockfd;
    fd.events = POLLIN;
    res = poll(&fd, 1, 10000); // 10000 ms timeout
    if (res == 0)
    {
        printf("Timeout!\n");
        return 0;
    }
    else if (res == -1)
    {
        printf("ERROR in receiving acknowledgement!\n");
        return 0;
    }
    else
    {
        //We can received the acknowledgement
        int n = recvfrom(sockfd, buf, BUFSIZE,0, &serveraddr, &serverlen);
        if(n<0){
            error("ERROR in recving acknowledgement!\n");
            return 0;
        }
        for(int i=0;i<n;i++){
            printf("%c",buf[i]);
        }
        printf("\n");
        return 1;
    }
}

void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd,n;
    int serverlen;
    struct hostent *server;
    char *hostname;
    char buf[8][BUFSIZE];
    char fname[50];
    int window_size=8;
    int count=0,flag=0;
    for(int i=0;i<8;i++)
        for(int j=0;j<BUFSIZE;j++)
            buf[i][j]='?';

    /* check command line arguments */
 
   
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* build the server's Internet address 
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);*/

    /* get a message from the user */
    printf("Enter file-name: ");
    scanf("%s",fname);
    printf("%s\n", fname);

    int fd=open(fname,O_RDONLY);
    struct stat stat_buf;
	fstat(fd, &stat_buf);
	int size = stat_buf.st_size;
	while ( (n = read(fd, buf[count%8]+4, BUFSIZE-24)) > 0) 	//reads from file and writes to the socket
	{
		//encode -->
		buf[count%8][0]=flag;
		int s1=n&255,s2=(n>>8)&255;
		buf[count%8][1]=s1;
		buf[count%8][2]=s2;
        buf[count%8][3]=count%8;
		// <-- encode
        printf("Sending package sequence %d\n",count%8);
        /*for(int i=0;i<BUFSIZE;i++)
            printf("%c",buf[count%8][i]);printf("\n");*/
		serverlen = sizeof(serveraddr);
        int n1 = sendto(sockfd, buf[count%8], BUFSIZE, 0, &serveraddr, serverlen);
        if (n1 < 0) 
          error("ERROR in sendto");

		if(count%window_size==7){
			//Ackwait
            while(!Ackwait(sockfd,serverlen)){
                printf("Acknowledgnment not recieved.\n");
                for(int i=0;i<8;i++){
                    serverlen = sizeof(serveraddr);
                    int n1 = sendto(sockfd, buf[i], BUFSIZE, 0, &serveraddr, serverlen);
                    if (n1 < 0) 
                      error("ERROR in sendto");
                }
            }
            printf("Acknowledgnment recieved.\n");
			flag=!flag;
            for(int i=0;i<8;i++)
                for(int j=0;j<BUFSIZE;j++)
                    buf[i][j]='?';
		}
		/* decode -->
		printf("#%d",buf[0]&255);
		int d1=buf[1]&255,d2=(buf[2]&255)<<8;
        printf("&%d",buf[3]&255);
		printf("^%d\n",d1|d2);
		// <-- decode*/
		count++;
	}
    char endS[BUFSIZE];
    endS[0]=flag;
    endS[1]=0;
    endS[2]=0;
    endS[3]=0;
    int n1 = sendto(sockfd, endS, BUFSIZE, 0, &serveraddr, serverlen);
    if (n1 < 0) 
      error("ERROR in sendto");
    while(!Ackwait(sockfd,serverlen)){
        printf("Acknowledgnment not recieved.\n");
        for(int i=0;i<8;i++){
            serverlen = sizeof(serveraddr);
            int n1 = sendto(sockfd, buf[i], BUFSIZE, 0, &serveraddr, serverlen);
            if (n1 < 0) 
              error("ERROR in sendto");
        }
    }
    printf("Acknowledgnment recieved.\n");

    /* send the message to the server */
    /*serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    
    /* print the server's reply 
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);*/
    return 0;
}
