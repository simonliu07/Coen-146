/*****************************
 * COEN 146, UDP, client
 *****************************/
/*	Simon Liu 
 *	Coen 146 Lab Friday 2:15pm
 */


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "tfv2.h"
#include <stdlib.h>
#include <time.h>

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
		recvfrom(sock,pktr,sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);	//wait for ack respond from server
		if(pktr->header.seq_ack == 0)								//if the ack is correct, continues with program
			break;
		sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);	//send the package again since it failed
		++temp;											//increment counter
	}
	int status = 1;		//value for client ack value
	int amount_read = 0;	//how bytes read
	int random = 0;		//value for deliberate error
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
			if(random <= 10){		//there is a 10% chance we will create an ack error
				printf("Fail ack sequence!!\n");
				pkt->header.seq_ack = !(pkt->header.seq_ack);
			}
			else if(random >= 97){		//completely fails the transfer with an incorrect checksum value, doesn't attempt to correct the error
				printf("Critical Failure!!!\n");
				pkt->header.checksum = 0;
			}
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, addr_size);		//send packet to server
			recvfrom(sock,pktr,sizeof(PACKET), 0, (struct sockaddr *)&serverAddr, &addr_size);		//wait for server respond
			if(random <= 10){	//if we are in error mode
				pktr->header.seq_ack = !(pktr->header.seq_ack);		//make sure the adjust the error since server doesn't know its in error mode
				pkt->header.seq_ack = status;		//correct the send packet with the correct ack for sending
			}
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
