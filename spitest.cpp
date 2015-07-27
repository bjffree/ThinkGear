#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <mysql/mysql.h>
#include <wiringPi.h>
#include <sstream>
#include "mcp3008Spi.h"
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>

#define THINKGEAR_SYNC 0xAA
#define THINKGEAR_MAX_DATA_SIZE 169
#define THINKGEAR_EXCODE 0x55
#define SINGLE_BYTE_THINKGEAR_CODE(code) (code <= 0x7F)
#define MULTI_BYTE_THINKGEAR_CODE(code) (code > 0x7F)

#define PAGE0_ENABLE_ATTENTION		0b00000001
#define PAGE0_ENABLE_MEDITATION		0b00000010
#define PAGE0_ENABLE_RAW_WAVE		0b00000100
#define PAGE0_ENABLE_HIGH_BAUD_RATE	0b00001000
#define SHELLSCRIPT "\
#!/bin/bash \n\
sudo rfcomm bind 0 74:E5:43:D5:70:37 1 \n\
"
//white band 74:E5:43:D5:70:37	
//brandons 20:68:9D:88:C5:93
#define PROBLEM_LED  0
#define WORKING_LED  6
#define READY_LED    3
#define READ_SWITCH  2
#define HEART_SENSOR 8
#define TEMP_SENSOR  7
#define piezoSpeaker 1 //need to include softTone.h

int serial;
FILE * output;
using namespace std;

void initBluetooth(){
	system(SHELLSCRIPT);
}
void initPins(){
	wiringPiSetup();
	pinMode (PROBLEM_LED,OUTPUT);
	pinMode (WORKING_LED,OUTPUT);
	pinMode (READY_LED  ,OUTPUT);
	pinMode (HEART_SENSOR,INPUT);
	pinMode (TEMP_SENSOR, INPUT);
	digitalWrite(PROBLEM_LED,LOW);
	digitalWrite(WORKING_LED,LOW);
	digitalWrite(READY_LED,LOW);
}
int readData(int a2dChannel){
		int a2dVal=0;
		mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8);
		 unsigned char temper[3];
		temper[0] = 1;  //  first byte transmitted -> start bit
        temper[1] = 0b10000000 |( ((a2dChannel & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
        temper[2] = 0; // third byte transmitted....don't care
         a2d.spiWriteRead(temper, sizeof(temper) );
 
 
				a2dVal = 0;
                a2dVal = (temper[1]<< 8) & 0b1100000000; //merge data[1] & data[2] to get result
                a2dVal |=  (temper[2] & 0xff);
               
        sleep(1);
        
       
	return a2dVal;
}float ConvertVolts(int data){
	float volts;
	volts = (data* 3.3) / 1023;
	volts = roundf(volts);
	return volts;
}
float ConvertTemp(int data){
	float temp;
	temp = ((data * 330) / float(1023))-50;
	temp = roundf(temp);
	return temp;
}
int main(int argc, char* argv[])
{
	bool connected = false,ready = true, reading = false;
	int heart = 0,temp = 0;
	
	
	initBluetooth();
	initPins();
	int out = digitalRead(READ_SWITCH);
	printf("out: %i\n",out);
	
    int i = 50;
        int a2dVal = 0;
    int a2dChannel;
       
 
    while(i > 0)
    {
        cout << "Volts : "<<ConvertVolts(readData(1))<<endl;
        i--;
    }
	
	return 0;
}

