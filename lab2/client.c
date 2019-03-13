 /**************************
	socket example, client
	Winter 2019
 **************************/
/*	Simon Liu COEN 146 Friday 2:15pm
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

int main (int, char *[]);


/********************
 *	main
 ********************/
int main (int argc, char *argv[])
{
	int i;
	int sockfd = 0, n = 0;
	char buff[10];
	char *p;
	struct sockaddr_in serv_addr; 

	if (argc != 5)	//make sure all 4 arguments are in
	{
		printf ("Usage: %s <ip of server> \n",argv[0]);
		return 1;
	} 

	// set up
	memset (buff, '0', sizeof (buff));
	memset (&serv_addr, '0', sizeof (serv_addr)); 

	// open socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf ("Error : Could not create socket \n");
		return 1;
	} 

	// set address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons (atoi(argv[1])); //let the socket # be argument

	if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0)	//set the ip address
	{
		printf ("inet_pton error occured\n");
		return 1;
	} 

	// connect
	if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
	{
		printf ("Error : Connect Failed \n");
		return 1;
	} 
	//send the destination name
	write(sockfd,argv[4], strlen(argv[4])+1);
	read(sockfd,buff,10);	//see if the destination file is created, if so there should be a respond
	
	// open source file
	FILE *source = fopen(argv[3],"rb");
	
	int amount_read = 0;
	// input, send to server, receive it back, and output it
	while ((amount_read = fread(buff, 1, 10, source)) != 0)
	{
		write (sockfd, buff, amount_read);	//keep sending 10 bytes to server
	} 
	close (sockfd);	//close client
	return 0;

}
