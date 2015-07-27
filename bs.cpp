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
struct connection_details{
	char *server;
	char *user;
	char *password;
	char *database;
};
MYSQL* mysql_connection_setup(struct connection_details mysql_details){
	MYSQL * connection = mysql_init(NULL);
	if (!mysql_real_connect(connection,mysql_details.server,mysql_details.user,
		mysql_details.password, mysql_details.database,0,NULL,0)){
			printf("Connection error : %s\n", mysql_error(connection));
			exit(1);
		}
		return connection;
}
MYSQL_RES* mysql_perform_query(MYSQL *connection, char *sql_query){
	if(mysql_query(connection, sql_query)){
		printf("MySQL query error : %s\n", mysql_error(connection));
		exit(1);
	}
	return mysql_use_result(connection);
}
void sigint_handler(int s)
{
	// Finish
	printf("\b\bOperation succeed.\n");
	close(serial);
	fclose(output);
	exit(0);
}
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
int main(int argc, char* argv[])
{
	bool connected = false,ready = true, reading = false;
	int heart = 0,temp = 0;
	
	
	initBluetooth();
	initPins();
	int out = digitalRead(READ_SWITCH);
	printf("out: %i\n",out);
	string data = "INSERT INTO (Mobiletest) values (time,heart,temp,delta,theta,alphaLow,alphaHigh,betaLow,betaHigh,gammaLow,gammaMid)";
	signal (SIGINT,sigint_handler);
	MYSQL *conn;
	struct connection_details mysqlD;
	mysqlD.server = "grid.uhd.edu";
	mysqlD.user = "brainwaveWebsite";
	mysqlD.password = "KX2mrRMeCyAM7Qpa";
	mysqlD.database = "brainwave";
	
	conn = mysql_connection_setup(mysqlD);
	// Open the port
	printf("Opening port.\n");
	//serial = open(argv[1], O_RDWR);
	serial = open("/dev/rfcomm0", O_RDWR);
	if (serial == -1)
	{
		printf("Cannot open the port:%s, errno = %i.\n", argv[1], errno);
		ready = false;
		exit(1);
	}

	// Open the file
	if (argc == 3)
	{
		output = fopen("(unsigned long long)time(NULL)", "w+");
	}
	else
	{
		output = fopen("/dev/null", "w+");
	}
	
	if (!output)
	{
		printf("Cannot open the file:%s, errno = %i.\n", argv[2], errno);
		ready = false;
		exit(2);
	}
	if (ready == false){
		digitalWrite(PROBLEM_LED, HIGH);
	}else
		ready = true;
	
	// Send control byte
	printf("Sending control byte.\n");
	unsigned char command = 0x00;
	if (write(serial, &command, sizeof(command)) < sizeof(command))
	{
		printf("Cannot write the serial.\n");
		ready = false;
		exit(3);
	}
	printf("Working.\n");
	while (true){
			do
			{
				
				// According to ThinkGear Serial Stream Guide the packet starts with the HEADER
				unsigned char HEADER = 0;

				// Read till the first SYNC
				if (read(serial, &HEADER, sizeof(HEADER)) < sizeof(HEADER))
				{
					printf("Cannot read the serial.\n");
					ready = false;
					exit(3);
				}
				if (HEADER != THINKGEAR_SYNC) continue;

				//Check if the next byte is SYNC
				if (read(serial, &HEADER, sizeof(HEADER)) < sizeof(HEADER))
				{
					printf("Cannot read the serial.\n");
					ready = false;
					exit(3);
				}
				if (HEADER != THINKGEAR_SYNC) continue;

				//Check if the next byte is PLENGTH
				while (HEADER == THINKGEAR_SYNC)
				{
					if (read(serial, &HEADER, sizeof(HEADER)) < sizeof(HEADER))
					{
						printf("Cannot read the serial.\n");
						ready = false;
						exit(3);
					}
				}
				if (HEADER > THINKGEAR_SYNC) continue;


///////////////////////////////////////////////////////////////////////////////////

	mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, 8);
    int i = 20;
        int a2dVal = 0;
    int tempChannel = 0;
        unsigned char temper[3];
 
    while(i > 0)
    {
        temper[0] = 1;  //  first byte transmitted -> start bit
        temper[1] = 0b10000000 |( ((tempChannel & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
        temper[2] = 0; // third byte transmitted....don't care
 
        a2d.spiWriteRead(temper, sizeof(data) );
 
        a2dVal = 0;
                a2dVal = (temper[1]<< 8) & 0b1100000000; //merge data[1] & data[2] to get result
                a2dVal |=  (temper[2] & 0xff);
        sleep(1);
        cout << "The Result is: " << a2dVal << endl;
        i--;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////



				// Read PAYLOAD
				unsigned char PAYLOAD[169];
				int checksum = 0;
				for (int i = 0; i < HEADER; i++)
				{
					if (read(serial, &PAYLOAD[i], sizeof(PAYLOAD[i])) < sizeof(PAYLOAD[i]))
					{
						printf("Cannot read the serial.\n");
						exit(3);
					}
					checksum += PAYLOAD[i];
				}
				checksum &= 0xFF;
				checksum = ~checksum & 0xFF;

				// Read CHKSUM
				unsigned char CHKSUM;
				if (read(serial, &CHKSUM, sizeof(CHKSUM)) < sizeof(CHKSUM))
				{
					printf("Cannot read the serial.\n");
					ready = false;
					exit(3);
				}
				time_t timestamp;
				if (CHKSUM != checksum && ready != false) continue;
				for (int i = 0; i < HEADER; i++)
				{
					digitalWrite(READY_LED,HIGH);
					if (PAYLOAD[i] == THINKGEAR_EXCODE)
					{
						printf("Unsupported code.\n");
						break;
					}
					else if (SINGLE_BYTE_THINKGEAR_CODE(PAYLOAD[i]))
					{
						switch (PAYLOAD[i++])
						{
						case 0x02:
							printf("SIGNAL: %i\n", PAYLOAD[i]);
							break;
						case 0x03:
							//printf("HEART_RATE: %i\n", PAYLOAD[i]);
							break;
						case 0x04:
							//printf("ATTENTION: %i\n", PAYLOAD[i]);
							break;
						case 0x05:
							//printf("MEDITATION: %i\n", PAYLOAD[i]);
							break;
						case 0x06:
							//printf("8BIT_RAW: %i\n", PAYLOAD[i]);
							break;
						case 0x07:
							//printf("RAW_MARKER: %i\n", PAYLOAD[i]);
							break;
						default:
							//printf("Unknown Code: %i\n", PAYLOAD[i]);
							break;
						}
					}
					else if (MULTI_BYTE_THINKGEAR_CODE(PAYLOAD[i]))
					{
						switch (PAYLOAD[i])
						{
						case 0x80:
							//printf("RAW Wave Value\n");
							break;
						case 0x81:
							//printf("EEG_POWER\n");
							break;
						case 0x83:
							
							int delta,theta,alpha1,alpha2,beta1,beta2,gamma1,gamma2;
							heart = digitalRead(HEART_SENSOR);
							temp  = digitalRead(TEMP_SENSOR);
							
							
							fprintf(output, "%llu,", (unsigned long long)time(NULL));
							delta = (PAYLOAD[i + 2] << 16) + (PAYLOAD[i + 3] << 8) + PAYLOAD[i + 4];
							theta = (PAYLOAD[i + 5] << 16) + (PAYLOAD[i + 6] << 8) + PAYLOAD[i + 7];
							alpha1 = (PAYLOAD[i + 8] << 16) + (PAYLOAD[i + 9] << 8) + PAYLOAD[i + 10];
							alpha2 = (PAYLOAD[i + 11] << 16) + (PAYLOAD[i + 12] << 8) + PAYLOAD[i + 13];
							beta1 = (PAYLOAD[i + 14] << 16) + (PAYLOAD[i + 15] << 8) + PAYLOAD[i + 16];
							beta2 = (PAYLOAD[i + 17] << 16) + (PAYLOAD[i + 18] << 8) + PAYLOAD[i + 19];
							gamma1 = (PAYLOAD[i + 20] << 16) + (PAYLOAD[i + 21] << 8) + PAYLOAD[i + 22];
							gamma2 = (PAYLOAD[i + 23] << 16) + (PAYLOAD[i + 24] << 8) + PAYLOAD[i + 25];
							if (out == 1){
								digitalWrite(WORKING_LED,HIGH);
								printf("%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i\n",heart,temp, delta,theta,alpha1,alpha2,beta1,beta2,gamma1,gamma2);
								
								stringstream statement;
								statement << "INSERT INTO MobileTest (time,temp,heart,delta,theta,alpha1,alpha2,beta1,beta2,gamma1,gamma2) values ('";
								statement << time <<"','";
								statement << temp <<"','";
								statement << heart <<"','";
								statement << delta <<"','";
								statement << theta <<"','";
								statement << alpha1 <<"','";
								statement << alpha2 <<"','";
								statement << beta1 <<"','";
								statement << beta2 <<"','";
								statement << gamma1 <<"','";
								statement << gamma2 <<"')";
								
								data=statement.str();
								int query_state=mysql_query(conn, data.c_str());
								if(query_state != 0){
									std::cout<<mysql_error(conn)<<std::endl;
									fprintf(output, "%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i,%8i\n",heart,temp, delta,theta,alpha1,alpha2,beta1,beta2,gamma1,gamma2);
								}
							}else{
								digitalWrite(WORKING_LED,LOW);
								break;
							}
						
						default:
							printf("Writing to file: %i\n", PAYLOAD[i]);
							break;
						}
						i += PAYLOAD[++i];
					}
				}out = digitalRead(READ_SWITCH);
			}while(out == 1);
	}
	return 0;
}

/***********************************************************************
 * mcp3008SpiTest.cpp. Sample program that tests the mcp3008Spi class.
 * an mcp3008Spi class object (a2d) is created. the a2d object is instantiated
 * using the overloaded constructor. which opens the spidev0.0 device with
 * SPI_MODE_0 (MODE 0) (defined in linux/spi/spidev.h), speed = 1MHz &
 * bitsPerWord=8.
 *
 * call the spiWriteRead function on the a2d object 20 times. Each time make sure
 * that conversion is configured for single ended conversion on CH0
 * i.e. transmit ->  byte1 = 0b00000001 (start bit)
 *                   byte2 = 0b1000000  (SGL/DIF = 1, D2=D1=D0=0)
 *                   byte3 = 0b00000000  (Don't care)
 *      receive  ->  byte1 = junk
 *                   byte2 = junk + b8 + b9
 *                   byte3 = b7 - b0
 *    
 * after conversion must merge data[1] and data[2] to get final result
 *
 *
 *
 * *********************************************************************/

 
  
