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
 * Reads a range of registers and outputs their values.
 *
 * Primarily used to generate modbus over tcp packets for analysis.
 *
 *****************************************************************************/

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

    if (argc != 6) {
        fprintf(stdout, "\nUsage: %s <ip> <port> <starting register> <ending register> <freq>\n\n", argv[0]);
        return -1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    uint16_t start_register = (uint16_t)strtoul(argv[3], NULL, 10);
    uint16_t end_register = (uint16_t)strtoul(argv[4], NULL, 10);
    int freq = atoi(argv[5]);
    int i;

    if( start_register < 0 ) {
        fprintf(stderr, "Invalid start register (%d)\n", start_register);
        return -1;
    }

    if( end_register < start_register ) {
        fprintf(stderr, "Invalid end register (%d)\n", end_register);
        return -1;
    }

    uint16_t num_registers = end_register - start_register + 1;
    uint16_t *register_values = malloc(num_registers * sizeof(*register_values));

    fprintf(stderr, " ip = %s\n port = %d\n start register = %u\n end register = %u\n freq = %d\n", ip, port, start_register, end_register, freq);

    modbus_t *ctx = modbus_new_tcp(ip, port);
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection to %s port %d failed: %s\n", ip, port, modbus_strerror(errno));
        modbus_end(ctx);
        return -1;
    }

    while (1) {

        // uses modbus function code 0x03 (read holding registers)
        //printf("requesting %d registers, %u through %u\n", end_register-start_register+1, start_register, end_register);
        int rc = modbus_read_registers(ctx, 0, (end_register - start_register + 1), register_values);
        if (-1 == rc) {
            fprintf(stderr, "Reading registers failed. %s\n", modbus_strerror(errno));
            modbus_end(ctx);
            return -1;
        }

        for(i = 0; i < (end_register - start_register + 1); i++) {
            fprintf(stdout, "%d\t%d\t", i + start_register, register_values[i]);
        }
        fprintf(stdout, "\n"); 

        if (freq == 0) break;
        
        sleep(freq);
    }

    modbus_end(ctx);
    
    free(register_values);

    return 0;
}
