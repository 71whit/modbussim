/*****************************************************************************
 * 
 * c 2016 Whit Schonbein
 *
 *****************************************************************************
 *
 * Author       : Whit Schonbein (schonbein [at] cs.unm.edu)
 * Institution  : University of New Mexico, Albuquerque
 * Year         : 2016
 * Course       : cs544
 *
 ***************************************************************************** 
 *
 * Purpose:
 *
 * Determines number of registers in an attached PLC
 *
 ****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <modbus.h>

int modbus_end(modbus_t *ctx) {
    modbus_close(ctx);
    modbus_free(ctx);
    return 0;
}

int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stdout, "\nUsage: %s <ip> <port> <max_num_registers>\n\n", argv[0]);
        return -1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int max_num_registers = atoi(argv[3]);
    
    uint16_t *register_values = malloc(sizeof(*register_values));

    fprintf(stderr, "Attempting to determine number of registers at:\n");
    fprintf(stderr, " ip = %s\n port = %d\n max number of registers = %d\n", ip, port, max_num_registers);

    modbus_t *ctx = modbus_new_tcp(ip, port);
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection to %s port %d failed: %s\n", ip, port, modbus_strerror(errno));
        modbus_end(ctx);
        return -1;
    }

    int cur_register = 0;
    int num_registers = 0;
    for ( cur_register = 0; cur_register < max_num_registers; cur_register++) {

        fprintf(stderr, "Testing register %d...", cur_register);

        int rc = modbus_read_registers(ctx, cur_register, 1, register_values);
        if (-1 == rc) {
            fprintf(stderr, "\nReading registers failed (modbus protocol exception number = %d). %s\n", errno - MODBUS_ENOBASE, modbus_strerror(errno));
            break;
        }

        fprintf(stderr, "Found!\n");
        ++num_registers;

        sleep(1);
    }

    fprintf(stdout, "PLC on %s port %d has %d registers.\n", ip, port, num_registers);

    modbus_end(ctx);
    
    free(register_values);

    return 0;
}
