/*****************************
 * COEN 146, UDP, client
 *****************************/
/*	Simon Liu 
 *	Coen 146 Lab Friday 2:15pm
 *	RDT3.0
 */


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "tfv2.h"
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/***********
 *  main
 ***********/
int main (int argc, char *argv[])
{
	int sock, portNum, nBytes;
	char buffer[10];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	if (argc != 5)
	{
		printf ("missing argument(s)\n");
		return 1;
	}

	// configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	/*Create UDP socket*/
    	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0){
		printf ("socket error\n");
		return 1;
	}

	//open destination file
	FILE *src = fopen(argv[3], "rb");
	if(src == NULL){
		printf("Fail to open source file\n");
		return 1;
	}

	//Create the rdt3.0 function, the timer
	struct timeval tv;
	int rv;
	fd_set readfds;
	fcntl(sock,F_SETFL,O_NONBLOCK);

	//send the destination name
	PACKET *pkt = malloc(sizeof(PACKET));		//create PACKET pointer for sending 
	PACKET *pktr = malloc(sizeof(PACKET));		//create PACKET pointer for receiving
	int temp = strlen(argv[4])+1;			//inf the length of the destination name
	pkt->header.length = temp;			//set the variables in the packet
	pkt->header.seq_ack = 0;			//set the initial ack value
	pkt->header.checksum = 0;			//reset the checksum value for calculation
	strcpy(pkt->data, argv[4]);			//copy the name into data of the packet
	pkt->header.checksum = calc_checksum(pkt, temp+ sizeof(HEADER));	//calculate the checksum value and save it in packet
	sendto(sock, pkt,sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);		//send the destination name to server
	temp = 0;
	//acknowledgement of the destination name, resend max 3 times
	while(temp < 3){
		FD_ZERO(&readfds);										//reset the values for the clock
		FD_SET(sock,&readfds);
		tv.tv_sec = 1;	//seconds
		tv.tv_usec = 0;	//micro seconds
		rv = select(sock+1, &readfds, NULL, NULL, &tv);							//see if there is a respond for the given time 
		if(rv == 0){											//if we receive nothing within the given time 
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);	// send the message again 
			++temp;
		}
		else if(rv == 1){										//if we do receive something in the given time 
			recvfrom(sock,pktr,sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);	//wait for ack respond from server
			if(pktr->header.seq_ack == 0){								//if the ack is correct, continues with program
				temp = 10;									//set the flag 
				break;
			}
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);	//send the package again since it failed
			++temp;											//increment counter
		}
	}
	if(temp != 10){												//if the flag indicate fail to send 
		printf("Fail to send destination name to server!\n");
		pkt->header.length = 0;
		memset(pkt->data, '\0', 1);
		sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);		//send a packet to end server
		return 1;											//end program
	}
	int status = 1;		//value for client ack value
	int amount_read = 0;	//how bytes read
	int random = 0;		//value for deliberate error
	int save_checksum = 0;
	srand(time(0));
	while ((amount_read = fread(buffer, 1, 10, src)) > 0)	//keep in the loop as long as there is something to read from source file
	{
		temp = 0;		//reset counter
		pkt->header.length = amount_read;	//find how many bytes read
		strcpy(pkt->data, buffer);		//copy the bytes into the data of packet
		pkt->header.seq_ack = status;		// get the right pakect sequence number
		pkt->header.checksum = 0;		//reset the checksum value for calculation
		pkt->header.checksum = calc_checksum(pkt, pkt->header.length + sizeof(HEADER));	//find the checksum value
		// send
		while(temp < 3){	//at most try three times
			random = rand()%100;		//get the random number
			if(random >= 95){		//transfer with an incorrect checksum value, attempts to correct
				printf("Fail checksum value!!!\n");
				save_checksum = pkt->header.checksum;		//create an error for checksum, 5% chance
				pkt->header.checksum = 0;
			}
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);		//send packet to server
			FD_ZERO(&readfds);										//reset the value of the timer
			FD_SET(sock, &readfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			rv = select(sock+1, & readfds, NULL, NULL, &tv);						//see if there is respond within timer
			if(rv == 0){											//if nothing is received, increase counter
				printf("Packet Loss, send again\n");
				++temp;
			}
			else if(rv == 1){										//if something is received
				recvfrom(sock,pktr,sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);		//wait for server respond
				if(random >=95)						
					pkt->header.checksum = save_checksum;		//correct the error of checksum
				if(pktr->header.seq_ack == status){			//if the server return a correct ack
					printf("Send sucessful!!\n");			//tell user the bytes are sent
					status = status == 0? 1:0;			//alternate the status
					temp = 10;					//flag it success and get out of the loop
					break;			
				}
				else{
					printf("Send Failed, Trying Again!!\n");	//tell user it failed and will try again
				}
				++temp;	//increase the counter
			}
		}
		if(temp != 10){	//if the program exit previous loop after 3 tries
			printf("Fail to connect with server!!!\n");	//failed the max attempt to send file 
			pkt->header.length = 0;			//set a packet length to 0, telling server to shut down
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);	//send to server
			return 1;	//shut down client
		}
	}
	pkt->header.length = 0;		//send the server instruction to shut down 
	memset(pkt->data, '\0', 1);
	sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);
	fclose(src);	//close the source file 

	return 0;	//shut down client
}
