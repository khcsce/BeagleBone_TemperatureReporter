//NAME: Khoa Quach
//EMAIL: khoaquachschool@gmail.com
//ID: 105123806
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
// new header files for Project 1a:
#include <termios.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <mraa.h>
#include <mraa/aio.h>
#include <mraa/gpio.h>

#define TEMP 1
#define BUTTON 60
#define B 4275 // thermistor value
#define R0 100000.0 // nominal base value
#define NS '\0'
#define TAB '\t'
#define SP ' '
// Farenheit or Celsius
char scale = 'F';
// Option flags
int period_flag = 0;
int scale_flag = 0;
int log_flag = 0;
// Default Period
int period = 1;

// Log file file descriptor
FILE *file = 0;
// mraa
mraa_aio_context temp;
mraa_gpio_context button;
// Polling
struct pollfd pollfds[1];
// Time Struct
struct timeval clk;
struct tm *time_now;
time_t time_after = 0;

// Print or not
int stopped = 0;
int start = 1;

void print_error(const char* msg){
	fprintf(stderr, "%s , error number: %d, strerror: %s\n ",msg, errno, strerror(errno));
    exit(1);
}
void print(char *message){
	//fprintf(stdout, "%s\n", str);
	//fflush(stdout);
	if (file || log_flag) {
		fprintf(file, "%s\n", message);
		fflush(file);
	}
}

float convert_temper_reading()
{
	float reading = mraa_aio_read(temp);
	float R = 1023.0/((float) reading) - 1.0;
	R = R0*R;
	//C is the temperature in Celcious
	float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
	//F is the temperature in Fahrenheit
	float F = (C * 9)/5 + 32;
	switch (scale){
		case 'F': return F;
		default: return C;
	}
}

void print_time(){
	double temp = convert_temper_reading();
	gettimeofday(&clk,0);
	if (!stopped && start && clk.tv_sec >= time_after){
		time_now = localtime(&clk.tv_sec);
		char buf[256];
		sprintf(buf, "%02d:%02d:%02d %.1f", time_now->tm_hour, 
    			time_now->tm_min, time_now->tm_sec, temp);
		fprintf(stdout, "%s\n", buf);
		fflush(stdout);
    	print(buf);
    	time_after = clk.tv_sec + period; 
	}
}

void shutdown(){
	time_now = localtime(&clk.tv_sec);
	char buf[256];
	sprintf(buf, "%02d:%02d:%02d SHUTDOWN", time_now->tm_hour, 
    	time_now->tm_min, time_now->tm_sec);
	fprintf(stdout, "%s\n", buf);
	fflush(stdout);
	print(buf);
	exit(0);
}


void setup_sensors(){
	temp = mraa_aio_init(TEMP);
	if (temp == NULL){
		mraa_deinit();
		print_error("Error: initializing AIO (temp sensor)");
	}
	button = mraa_gpio_init(BUTTON);
	if (button == NULL){
		mraa_deinit();
		print_error("Error: initializing GPIO_50 (button)");
	}
	mraa_gpio_dir(button, MRAA_GPIO_IN);
}

void setup_poll(){
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN | POLLHUP | POLLERR;
}


int equal_command(char *command,char *str) {
	return strcmp(command,str) == 0;
}

int start_with(char *command,char *str){
	return strncmp(command,str,strlen(str)) == 0;
}

void process_commands(char *command){
	//https://piazza.com/class/kirz3jfa5jv7l7?cid=801
	// 1) poll for stdin, read into buffer
	//2) parse different lines in buffer, skipping spaces and tabs ONLY AT BEGINNING of each line
	command[strlen(command) - 1] = NS;
	while (*command == SP || *command == TAB){
		command++; // skip space and tabs !!!!
	}
	if(equal_command(command,"SCALE=F")){
		print(command);
		scale = 'F';
	} else if (equal_command(command,"SCALE=C")){
		print(command);
		scale = 'C';
	} else if (equal_command(command,"OFF")){
		print(command);
		shutdown();
	} else if (equal_command(command,"STOP")){
		print(command);
		start = 0;
		stopped = 1;
	} else if (equal_command(command,"START")){
		print(command);
		start = 1;
		stopped = 0;
	} else if (start_with(command,"LOG")){
		print(command);
	} else if (start_with(command,"PERIOD=")){
		period = atoi(&command[7]);
		print(command);
	}
}

void poll_and_commands(){
	char command[1024];
	while(1){
		print_time();
		int ret = poll(pollfds, 1, 1000);
		if (ret < 0) {
			print_error("Error: polling");
		}
		if (ret >= 1 /*pollfds[0].revents & POLLIN*/) {
			fgets(command, 1024, stdin);
			process_commands(command);
		}
		if (mraa_gpio_read(button)){
			shutdown();
		}
	}
}



int main(int argc, char **argv){
	int choice;
	static struct option long_options[] = 
	{
		{"period", 1, 0, 'p'}, // no argument
		{"scale", 1, 0, 's'},
		{"log",1,0,'l'},
		{0,0,0,0}
	};


	while((choice = getopt_long(argc, argv, "", long_options,NULL))!= -1)
	{
		switch(choice){
			case 'p':
				period_flag = 1;
				period = atoi(optarg);
				break;
			case 'l':
				log_flag = 1;
				file = fopen(optarg, "w+");
            	if(file == NULL) {
            		fprintf(stderr, "Logfile invalid\n");
					exit(1);
				}
				break;
			case 's':
				scale_flag = 1;
				if (optarg[0] == 'F' || optarg[0] == 'C'){
					scale = optarg[0];
				} else {
					print_error("Error: invalid scale type");
				}
				break;
			default:
				fprintf(stderr, "unrecognized argument! Correct usage: ./lab1a --shell (--shell is optional)\n");
				exit(1);
		}// switch
	}// while

	setup_sensors();
	setup_poll();
	poll_and_commands();
	// Close
	mraa_aio_close(temp);
	mraa_gpio_close(button);
	exit(0);
}
