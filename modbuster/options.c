/**********************************************************************
!*
!* 2016 Evan Dye  All Rights Reserved.
!*
!**********************************************************************
!*
!* Author       : Evan Dye
!* Institution  : University of New Mexico
!* Course       : CS-544
!* Semester     : Spring 2016
!*
!*********************************************************************
!******************************************************************************
! Purpose:   Functions used for command line options
!
!*******************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h> //For IP check

#include "options.h"

bool isValidIpAddress(char *ipAddress)
{
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
	return result != 0;
}

void printUsageInfo(void) {
	fprintf(stdout, "Usage: attack_modbussim.exe required_options [options]\n");
	fprintf(stdout, "Required Options:\n");
	fprintf(stdout, "\t-i <ip_address> \t Connect to PLC at IP address <ip_address>\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-r <register> \t\t Write to PLC's register number <register>. Default is -1 for auto register scan\n");
	fprintf(stdout, "\t-t <rpm> \t\t RPM <rpm> of target register. Default is 5000\n");
	fprintf(stdout, "\t-p <port> \t\t Connect to PLC on port number <port>. Default 0 for auto port scan\n");
	fprintf(stdout, "\t-h \t\t\t Display this information\n");
}


void get_options(int argc, char **argv, options_t *options)
{
	bool throwError = false;
	extern char *optarg;
	extern int optind;
	int optionCase;

	//Set values for required input so errors are caught
	options->ipAddress[0] = '\0';
	//Set defaults for non required input
	options->port = 0;
	options->registerAddress = -1;
	options->targetRPM = 5000;

	while((optionCase = getopt(argc,argv, "i:t:r:p:h")) != -1)
	{
		switch(optionCase)
		{
			case 'i':
				snprintf(options->ipAddress, 15, "%s", optarg); //Copy 15 characters of the string into ipAddress
				break;
			case 'p':
				options->port = atoi(optarg);
				break;
			case 'r':
				options->registerAddress = atoi(optarg);
				break;
			case 'h':
				printUsageInfo();
				exit(-1);
				break;
            case 't':
                options->targetRPM = atoi(optarg);
                break;
			case '?':
				fprintf(stderr, "Missing options");
				printUsageInfo();
				exit(-1);
			default:
				fprintf(stderr, "Unkown option: -%c\n", optionCase);
				break;
		}
	}

	options->tolerence = (options->targetRPM)/10;

	if(options->port < 0 || options->port > 65535)
	{
		fprintf(stderr, "Option -p %d \t\t is invalid\n", options->port);
		throwError = true;
	}

	if(!isValidIpAddress(options->ipAddress))
	{
		fprintf(stderr, "Option -i %s \t is invalid\n", options->ipAddress);
		throwError = true;
	}

	if(options->registerAddress < -1)
	{
		fprintf(stderr, "Options -r %d \t is invalid\n", options->registerAddress);
		throwError = true;
	}
	if(options->targetRPM < 0)
	{
		fprintf(stderr, "Options -t %d \t is invalid\n", options->targetRPM);
		throwError = true;
	}

	if(throwError)
	{
		printUsageInfo();
		exit(-1);
	}
}


void print_options(options_t *options)
{
	fprintf(stdout, "Using the following options:\n");
	fprintf(stdout, "\t IP Address: %s\n", options->ipAddress);
	fprintf(stdout, "\t Port: %d\n", options->port);
	fprintf(stdout, "\t Register Address: %d\n", options->registerAddress);
	fprintf(stdout, "\t Target RPM: %d\n", options->targetRPM);
	fprintf(stdout, "\t Target RPM Tolerence: %d\n", options->tolerence);
}

