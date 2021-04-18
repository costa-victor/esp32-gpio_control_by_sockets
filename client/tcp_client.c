/**
 * @file tcp_client.c
 * @author Victor Alberti Costa (victor.alberti.costa@gmail.com)
 * @brief TCP client application that sends commands
 * to ESP32 using sockets
 * @version 0.1
 * @date 2021-04-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Simple macro
#define CHECK_STATUS(x)         ( server_response & (x) )

// ID Commands recived from client side
#define CMD_RED_LED_ON          (1U << 0)
#define CMD_RED_LED_OFF         (1U << 1)
#define CMD_GREEN_LED_ON        (1U << 2)
#define CMD_GREEN_LED_OFF       (1U << 3)
#define CMD_BLUE_LED_ON         (1U << 4)
#define CMD_BLUE_LED_OFF        (1U << 5)


int main(int argc , char *argv[])
{
    int ledStatus = 0;
	int socket_desc;
    int command;
    int server_response = CMD_RED_LED_ON | CMD_GREEN_LED_ON | CMD_BLUE_LED_ON;
    int opt = -2;
	struct sockaddr_in server;
	

    printf("============== START ==============\n");
    
	// Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("YOUR-ESP-IP-ADDRESS");
	server.sin_family = AF_INET;
	server.sin_port = htons(3333);

	// Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		printf("\nSTATUS: Failed to connect to the ESP32\n");
		return 1;
	}
	
	printf("STATUS: Connected to ESP32\n\n");
	
    // A few seconds to read the status
    for (int i = 3; i >= 0; i--) {
        sleep(1);
        printf("Starting in %is\n", i);
    }
    system("clear");


    while(opt != -1){
        // If isn't start value, recieve the server response
        if(opt != -2){
            //Receive a reply from the server
            if( recv(socket_desc, (int *)&server_response, sizeof(server_response), 0) < 0){
                system("clear");
                printf("Recieve failed\n");
                sleep(1);                
            }
        }

        // Prints the menu
        system("clear");
        opt = 0;
        if(CHECK_STATUS(CMD_RED_LED_ON))
            printf("\n\n1. Turn ON  - RED LED\n");
        if(CHECK_STATUS(CMD_RED_LED_OFF))
            printf("\n\n1. Turn OFF - RED LED\n");            
        if(CHECK_STATUS(CMD_GREEN_LED_ON))
            printf("2. Turn ON  - GREEN LED\n");  
        if(CHECK_STATUS(CMD_GREEN_LED_OFF))
            printf("2. Turn OFF - GREEN LED\n");              
        if(CHECK_STATUS(CMD_BLUE_LED_ON))
            printf("3. Turn ON  - BLUE LED\n\n"); 
        if(CHECK_STATUS(CMD_BLUE_LED_OFF))
            printf("3. Turn OFF - BLUE LED\n\n");               


        // Reads the chosen option
        printf("Chosen option: ");
        scanf("%i", &opt);

        // Set the chosen command
        if(opt == 1)
            command = CHECK_STATUS(CMD_RED_LED_ON) ? CMD_RED_LED_ON : CMD_RED_LED_OFF;
        else if(opt == 2)
            command = CHECK_STATUS(CMD_GREEN_LED_ON) ? CMD_GREEN_LED_ON : CMD_GREEN_LED_OFF;
        else if(opt == 3)   
            command = CHECK_STATUS(CMD_BLUE_LED_ON) ? CMD_BLUE_LED_ON : CMD_BLUE_LED_OFF;

        // Send chosen command
        if( send(socket_desc , &command , sizeof(command) , 0) < 0){
            system("clear");
            printf("Send failed...");
            sleep(1);
	    }
	
    }
    
    system("clear");
    printf("\n============== END ==============\n");

	return 0;
}