/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFSIZE 1000
#define portno 820

/*
 * error - wrapper for perror
 */


void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[8][BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval,size[8],c_flag=-1,p_flag=-1,seq_no[8],count=0,filec=1; /* flag value for setsockopt */
  int n; /* message byte size */
  for(int i=0;i<8;i++)
    for(int j=0;j<BUFSIZE;j++)
      buf[i][j]='?';

  /* 
   * check command line arguments 
   */
  
  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  int fd=open("RecFile",O_WRONLY|O_CREAT,S_IRWXU);
  
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    n = recvfrom(sockfd, buf[count%8], BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);

    //printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    c_flag=buf[count%8][0]&255;
    int d1=buf[count%8][1]&255,d2=(buf[count%8][2]&255)<<8;
    size[count%8]=d1|d2;
    seq_no[count%8]=buf[count%8][3]&255;
    printf("flag: %d\tsize:%d\tseq_no:%d\tcontent:",c_flag,size[count%8],seq_no[count%8]);
    for(int i=0;i<size[count%8];i++){
      printf("%c",buf[count%8][i+4]);
    }printf("\n");

    if(seq_no[count%8]==0 && size[count%8]==0){
      printf("Sending Last Acknowledgment...\n");
      char msg[50]="Datagram received at server";
      int m = sendto(sockfd, msg, 50, 0, 
         (struct sockaddr *) &clientaddr, clientlen);
      if (m < 0) 
        error("ERROR in sendto");
        
        for(int i=0;i<(count%8);i++){
          for(int j=i+1;j<(count%8);j++){
            if(seq_no[i]>seq_no[j]){
              int tmp=seq_no[i];
              seq_no[i]=seq_no[j];
              seq_no[j]=tmp;
              tmp=size[i];
              size[i]=size[j];
              size[j]=tmp;
              char t1[BUFSIZE];
              strcpy(t1,buf[i]);
              strcpy(buf[i],buf[j]);
              strcpy(buf[j],t1);
            }     
          }
        }
        for(int i=0;i<(count%8);i++){
          write(fd,buf[i]+4,size[i]);
          /*for(int j=0;j<size[i];j++)
            printf("%c",buf[i][j+4]);
          printf("\n");*/

        }
      
      for(int i=0;i<8;i++)
        for(int j=0;j<BUFSIZE;j++)
          buf[i][j]='?';
        p_flag=c_flag;
        //close(fd);
        /*char fname[50]="RecFile";
        char c[]="0";
        c[0]='0'+filec;
        strcat(fname,c);
        printf("File Transfer Completed.\nConnection closed for current client.\n New file %s created for new session.\n",fname);
        fd=open(fname,O_WRONLY|O_CREAT,S_IRWXU);filec++;*/
        count=0;
    }

    if(count%8==7){
      printf("Sending Acknowledgment...\n");
      char msg[50]="Datagram received at server";
      int m = sendto(sockfd, msg, 50, 0, 
         (struct sockaddr *) &clientaddr, clientlen);
      if (m < 0) 
        error("ERROR in sendto");
      if(c_flag!=p_flag){
        for(int i=0;i<8;i++){
          for(int j=i+1;j<8;j++){
            if(seq_no[i]>seq_no[j]){
              int tmp=seq_no[i];
              seq_no[i]=seq_no[j];
              seq_no[j]=tmp;
              tmp=size[i];
              size[i]=size[j];
              size[j]=tmp;
              char t1[BUFSIZE];
              strcpy(t1,buf[i]);
              strcpy(buf[i],buf[j]);
              strcpy(buf[j],t1);
            }     
          }
        }
        for(int i=0;i<8;i++){
          write(fd,buf[i]+4,size[i]);
          /*for(int j=0;j<size[i];j++)
            printf("%c",buf[i][j+4]);
          printf("\n");*/

        }
      }
      for(int i=0;i<8;i++)
        for(int j=0;j<BUFSIZE;j++)
          buf[i][j]='?';
        p_flag=c_flag;
    }

    count++;
    /* 
     * sendto: echo the input back to the client 
     
    n = sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) 
      error("ERROR in sendto");*/
  }
}
