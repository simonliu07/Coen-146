 /**************************
 *     socket example, server
 *     Winter 2019
 ***************************/
/*	Simon Liu COEN 146L Friday 2:15pm
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

int main (int, char *[]); 


/*********************
 * main
 *********************/
int main (int argc, char *argv[])
{
	int n;
	char *p; 
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	char buff[10];
	FILE *destination = NULL;

	// set up
	memset (&serv_addr, '0', sizeof (serv_addr));
	memset (buff, '0', sizeof (buff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons (atoi(argv[1])); 

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); 
	listen (listenfd, 10); 
	
	// accept and interact
	while (1)
	{
		connfd = accept (listenfd, (struct sockaddr*)NULL, NULL); //set up connection
		read(connfd, buff, sizeof(buff));	//read for the destination name
		destination = fopen(buff, "wb");	//open a file with the designated name
		p = buff;				//set the buff to a char pointer
		*p++ = '1';				//set a designated value to the buff
		*p = '\0';				//set the end-string marker
		write(connfd, buff, 10);		//send it back to client
		while ((n = read (connfd, buff, sizeof (buff))) > 0)	//keep going as long as client send data
		{
			fwrite(buff, 1, n, destination); //write it into the file
		}
		fclose(destination);	//close the file
		close (connfd);		//close the connection
	}

	return 0;
}
