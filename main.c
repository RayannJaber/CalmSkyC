/*
 * Demonstration on how to redirect stdio to UART. 
 *
 * http://appelsiini.net/2011/simple-usart-with-avr-libc
 *
 * To compile and upload run: make clean; make; make program;
 * Connect to serial with: screen /dev/tty.usbserial-*
 *
 * Copyright 2011 Mika Tuupola
 *
 * Licensed under the MIT license:
 *   http://www.opensource.org/licenses/mit-license.php
 *
 */

//BAUD Defined in uart.h
#include "includes.h"
#include <string.h>

#include <stdio.h>
#include "avr/io.h"

#include "main.h"
#include "uart.h"




/* Name: main.c
 * Author: R. Scheidt
 * Copyright: <insert your copyright message here>
 * License: <insert your license reference here>
 */

#define BAUD 57600UL
#define F_CPU 16000000UL

#define NO_SYSTEM_ERROR						0
#define MEDIUM_PRIORITY_ERROR			1
#define HIGH_PRIORITY_ERROR				2

#define TASK_STK_SIZE							64		/* Size of each task's stacks (# of WORDs)            */
#define TRANSMIT_TASK_STK_SIZE		128		/* Size of the Transmit Task's stack                  */
#define TRANSMIT_BUFFER_SIZE			40	  /* Size of buffers used to store character strings    */

#define MAXPAYLOAD 169
#define EEG_SIZE 8

/*
 *********************************************************************************************************
 *                                               VARIABLES
 *********************************************************************************************************
 */
char* _eeg[EEG_SIZE];
char* payloadData[MAXPAYLOAD];
int poorQuality;
int attention;
int meditation;

OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_STK        TaskLedStk[TASK_STK_SIZE];
OS_STK        TaskTimerStk[TRANSMIT_TASK_STK_SIZE];
OS_STK        SerialTransmitTaskStk[TRANSMIT_TASK_STK_SIZE];

OS_EVENT     	*LedSem;
OS_EVENT     	*LedMBox;
OS_EVENT	 		*SerialTxSem;
OS_EVENT     	*SerialTxMBox;
OS_EVENT		*SerialRxMBox;

char RXStream[80];
int RXIndex = 0;

/*
 *********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

extern void InitPeripherals(void);

void  TaskStart(void *data);                  /* Function prototypes of Startup task */
void  NeuroSkyTask(void *data);                    /* Function prototypes of tasks   */
void  SerialTransmitTask(void *data);          /* Function prototypes of print task */

void  USART_TX_Poll(unsigned char pdata);	   	/* Function prototypes of LedTask */  
void 	PostTxCompleteSem (void);               /* Function prototypes of tasks */


/*
 *********************************************************************************************************
 *                                                MAIN
 *********************************************************************************************************
 */
int main (void)
{

	//getchar();
	InitPeripherals();

    OSInit();                                              /* Initialize uC/OS-II                      */

/* Create OS_EVENT resources here  */
	LedMBox = OSMboxCreate(NULL);
	SerialTxMBox = OSMboxCreate(NULL);
	SerialTxSem = OSSemCreate(1);
	DDRB = 0xFF; //port 5 is led bit
/* END Create OS_EVENT resources   */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 5);

    OSStart();                                             /* Start multitasking                       */

	while (1)
	{
		;
	}
}


/*
 *********************************************************************************************************
 *                                              STARTUP TASK
 *********************************************************************************************************
 */
void TaskStart (void *pdata)
{
    pdata = pdata;                                         /* Prevent compiler warning                 */

	OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	OSTaskCreate(NeuroSkyTask, (void *)0, &TaskTimerStk[TRANSMIT_TASK_STK_SIZE - 1], 30);
	//OSTaskCreate(CustomTask,NULL,&TaskLedStk[TASK_STK_SIZE - 1], 7);
	//OSTaskCreate(SerialTransmitTask, (void *) 0, &SerialTransmitTaskStk[TRANSMIT_TASK_STK_SIZE-1], 20);

    for (;;) {
        OSCtxSwCtr = 0;                         /* Clear context switch counter             */
        OSTimeDly(OS_TICKS_PER_SEC);			/* Wait one second                          */
    }
}

//Sends data from the SerialTxMBox to the UART to get printed
void SerialTransmitTask (void *pdata)
{
	void* err;
	char* msg;
	INT8U index=0;
	//char LocalMessage[TRANSMIT_BUFFER_SIZE];
	//OSTimeDly(OS_TICKS_PER_SEC);
	for (;;) {
		msg = (char*)OSMboxPend(SerialTxMBox,0,err);
		while(msg[index] != '\0'){
			//loop_until_bit_is_set(UCSR0A,UDRE0);
			if(msg[index] == '\n'){ //if newline, add a carriage return first
				UDR0 = '\r';
				//loop_until_bit_is_set(UCSR0A,UDRE0);
				OSSemPend(SerialTxSem,0,NULL);
			}
			UDR0 = msg[index];
			index++;
			OSSemPend(SerialTxSem,0,NULL);
			
		}
		index = 0;
		
	}
}
//Print ucos, uses The SerialTxMBox to print to console
void print_u(char* str){
	
}

void NeuroSkyTask(void *pdata) {    

	stdin = &uart_input;
	stdout = &uart_output;
	uart_init();
    char genchecksum =0; 
	char checksum =0;
	int payload = 0;         
    char input;
	DDRB = 0xFF;
    while(1) {
		//OSTimeDly(1);
		input = getchar();
		//UDR0 = 'c';


		
		if(input == 170){ //start byte
			input = getchar();			
			if(input == 170){
				//printf("Scanning!\r\n");
				payload = getchar();
				if(payload > MAXPAYLOAD)                      //Payload length can not be greater than 169
					continue;
				genchecksum =0;
				
				//memset(payloadData,0,sizeof(payloadData[0])*MAXPAYLOAD); //clear array for use
				//for(int i =0; i < MAXPAYLOAD; i++)
				//	payloadData[i] = 0;
				//printf("payload: %u\r\n ",payload);
				for(int i =0; i < payload; i++){
					input = getchar(); //read next [payload] bytes and save them		
					payloadData[i] = input;
					genchecksum += input;					
				}
				checksum = getchar();
				genchecksum = 255 - genchecksum; //take 1's compliment value
				if(checksum != genchecksum){
					//checksum error. ignore packet :(
					//printf("\r\n ERROR: gen: %u, check: %u\r\n",genchecksum,checksum);
					continue;
				}
				//printf("Success!\r\n");
				poorQuality = 200;
				attention = 0;
				meditation = 0;
				for(int i =0; i < payload; i++){				
					//printf("%u ",input);
					//continue;
					switch((int)payloadData[i]){
						case 2:
							i++;
							poorQuality = payloadData[i];
							break;
						case 4:
							i++;
							attention = payloadData[i];
							break;
						case 5:
							i++;
							meditation = payloadData[i];
							
							printf("\r\nMeditation: %u\r\n",meditation);
							//print_u(RXStream);
							//UDR0 = 'c';
							//return;
							break;
						case 0x80:
							i +=3; //????
							break;
						case 0x83:
							i++;
							i++;
							for (int j = 0; j < EEG_SIZE; j++) {
							
								_eeg[j] = ((int)payloadData[++i] << 16) | ((int)payloadData[++i] << 8) | (int)payloadData[++i];
							}
							//printf("\r\n\n");
							//printf("Delta: %u\r\n",delta()); //_eeg[0]
							//printf("Theta: %u\r\n",theta()); //_eeg[1]
							//printf("lowAlpha: %u\r\n",lowAlpha()); //_eeg[2]
							//printf("highAlpha: %u\r\n",highAlpha()); //_eeg[3]
							//printf("lowBeta: %u\r\n",lowBeta()); //_eeg[4]
							//printf("highBeta: %u\r\n",highBeta()); //_eeg[5]
							//printf("lowGamma: %u\r\n",lowGamma()); //_eeg[6]
							//printf("midGamma: %u\r\n",midGamma()); //_eeg[7]
							//printf("\r\n");
							break;
						default:
							break;
					}
				}


			}

		}
		//if(input == '\r')putchar('\n');
		//putchar(input);

		//if(input >='0' && input <= '7'){
			//pin = input-'0';		
			//PORTB ^= (1 << pin);
		//}     
		//printf("%X",input);
    }
        
    return 0;
}



int delta(){
	return _eeg[0];
}
int theta(){
	return _eeg[1];
}
int lowAlpha(){
	return _eeg[2];
}
int highAlpha(){
	return _eeg[3];
}
int lowBeta(){
	return _eeg[4];
}
int highBeta(){
	return _eeg[5];
}
int lowGamma(){
	return _eeg[6];
}
int midGamma(){
	return _eeg[7];
}


//ISR(USART_TX_vect)
//{
	//PostTxCompleteSem();
//}

/*
 *********************************************************************************************************
 *                        Routine to Post the Transmit buffer empty semaphone
 *********************************************************************************************************
 */
void PostTxCompleteSem (void)
{
	OSSemPost(SerialTxSem);
}