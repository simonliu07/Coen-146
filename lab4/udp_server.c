
/********************
 * COEN 146, UDP example, server    Simon Liu Coen 146 Lab 4 Fri 2:15pm
 * RDT 3.0
 ********************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include "tfv2.h"
#include <time.h>

/********************
 * main
 ********************/
int main (int argc, char *argv[])
{
	int sock, nBytes;
	char buffer[10];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;

    if (argc != 2)
    {
        printf ("need the port number\n");
        return 1;
    }

	// init 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0)
	{
		printf ("bind error\n");
		return 1;
	}
	
	//getting the file name
	PACKET *pkt = malloc(sizeof(PACKET));		//create a PACKET pointer
	int temp = 0;
	int status = 1;		//set the sequence number the server should recieve from the client
	while(temp < 3){	//the server will try to get the file name for only three tries
		recvfrom(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);	//get packet from client
		nBytes = pkt->header.checksum;	//save the packet checksum value
		pkt->header.checksum = 0;	//reset the checksum value for calculation
		pkt->header.checksum = calc_checksum(pkt, sizeof(HEADER)+pkt->header.length);		//caculate the checksum value of the packet
		if(pkt->header.seq_ack != 0 || pkt->header.checksum != nBytes){	//if the ack or checksum of the packet isn't correct
			pkt->header.seq_ack = !status;		//set the packet ack with the opposite of its original checksum value
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);	// send it back to client
			++temp;	//increase counter
		}
		else{	//if it is correct
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);	//tell the client the packet is correct
			temp = 10;	//get out of the loop 
			break;
		}
	}
	if(temp != 10)	//if all three attempts still doesn't get the destination name, close server
		return 0;
	FILE *dest = fopen(pkt->data, "wb");	//open destination file with the name it recieve
	
	srand(time(0));
	int random = 0;
	while (1)
	{
		// receive  datagrams
		recvfrom (sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, &addr_size);		//get the packet from the client
		random = rand()%100;
		if(pkt->header.length == 0)	//if the packet length is 0, this mean client want server to shut down 
			break;
		if(random > 10 && random < 20){		//create a 10% chance that there is packet loss
			printf("Packet Loss\n");	//do not respond to client 
		}
		else{
			nBytes = pkt->header.checksum;	//save the packet checksum value
			pkt->header.checksum = 0;	//reset checksum for calculationg
			pkt->header.checksum = calc_checksum(pkt, sizeof(HEADER) + pkt->header.length);		//calcuate the packet checksum
			if(pkt->header.seq_ack != status || pkt->header.checksum != nBytes){			//if the ack or checksum isn't correct
				pkt->header.seq_ack = !(pkt->header.seq_ack);					//send client a nack
			}
			else{	//if its correct
				fwrite(pkt->data, 1, pkt->header.length, dest);					//copy the data into destination file
				status = status == 0 ? 1:0;							//alternate the status of the server
			}
			sendto(sock, pkt, sizeof(PACKET), 0, (struct sockaddr *)&serverStorage, addr_size);	//send an ack back to client
		}
	}
	fclose(dest);	//close the destination to save its content 
	return 0;	//clost the server
}
