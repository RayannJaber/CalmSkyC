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
#define F_CPU 16000000UL
#include <stdio.h>
#include "avr/io.h"
#include "avr/delay.h"
#include "main.h"
#include "uart.h"

#define MAXPAYLOAD 169
#define EEG_SIZE 8
char* _eeg[EEG_SIZE];
char* payloadData[MAXPAYLOAD];
int poorQuality;
int attention;
int meditation;

int main(void) {    

    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
    char genchecksum =0; 
	char checksum =0;
	int payload = 0;         
    char input;
	DDRB = 0xFF;
    while(1) {
		input = getchar();

		if(input == 170){ //start byte
			input = getchar();
			if(input == 170){
				payload = getchar();
				if(payload > MAXPAYLOAD)                      //Payload length can not be greater than 169
					continue;
				genchecksum =0;
				
				//memset(payloadData,0,sizeof(payloadData[0])*MAXPAYLOAD); //clear array for use
				//for(int i =0; i < MAXPAYLOAD; i++)
				//	payloadData[i] = 0;

				for(int i =0; i < payload; i++){
					input = getchar(); //read next [payload] bytes and save them		
					payloadData[i] = input;
					genchecksum += input;
					
				}
				checksum = getchar();
				genchecksum = 255 - genchecksum; //take 1's compliment vlaue
				if(checksum != genchecksum){
					//checksum error. ignore packet :(
					//printf("\r\n ERROR: gen: %u, check: %u\r\n",genchecksum,checksum);
					continue;
				}
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
							printf("Delta: %u\r\n",delta()); //_eeg[0]
							printf("Theta: %u\r\n",theta()); //_eeg[1]
							printf("lowAlpha: %u\r\n"lowAlpha()); //_eeg[2]
							printf("highAlpha: %u\r\n",highAlpha()); //_eeg[3]
							printf("lowBeta: %u\r\n",lowBeta()); //_eeg[4]
							printf("highBeta: %u\r\n",highBeta()); //_eeg[5]
							printf("lowGamma: %u\r\n",lowGamma()); //_eeg[6]
							printf("midGamma: %u\r\n",midGamma()); //_eeg[7]
							printf("\r\n");
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
