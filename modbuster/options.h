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
! Purpose:   Header for functions/datatypes used for command line options
!
!*******************************************************************************/

#ifndef __OPTIONS__H__
#define __OPTIONS__H__


typedef struct options_s
{
	char ipAddress[15];
	int port;
	int registerAddress;
	int targetRPM;
	int tolerence;
} options_t;


void printUsageInfo(void);


void get_options(int argc, char **argv, options_t *options);


void print_options(options_t *options);


#endif //#ifndef __OPTIONS__H__

