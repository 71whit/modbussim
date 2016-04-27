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
 * Attempts to generate modbus exceptions to test whether what is returned 
 * is the correct exception code.
 *
 * Currently only checks modbus_read_registers error numbers.
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

    if (argc != 3) {
        fprintf(stdout, "\nUsage: %s <ip> <port>\n\n", argv[0]);
        return -1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int rc;    

    uint16_t *register_values = malloc(sizeof(*register_values));


    modbus_t *ctx = modbus_new_tcp(ip, port);
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection to %s port %d failed: %s\n", ip, port, modbus_strerror(errno));
        modbus_end(ctx);
        return -1;
    }

    /* generate exception 02 (starting address out of range) */
    fprintf(stdout, "Test #1: Asking for out of bounds register (expected exception number 02)\n");
    rc = modbus_read_registers(ctx, MODBUS_MAX_READ_REGISTERS+1, 1, register_values);
    if (-1 == rc) {
        fprintf(stderr, "Reading registers failed (modbus protocol exception number = %d). %s\n", errno - MODBUS_ENOBASE, modbus_strerror(errno));
    }

    sleep(1);

    /* generate exception 02 (range of addresses requested out of range) */
    /* Note: if the range goes beyond the protocol maximum, the returned errno will be 16, which is not a 
     * protocol exception code */
    fprintf(stdout, "Test #2: Asking for out of bounds range of registers (expected exception number 02)\n");
    rc = modbus_read_registers(ctx, 0, MODBUS_MAX_READ_REGISTERS-1, register_values);
    if (-1 == rc) {
        fprintf(stderr, "Reading registers failed (modbus protocol exception number = %d). %s\n", errno - MODBUS_ENOBASE, modbus_strerror(errno));
    }

    sleep(1);

    /* generate exception 03 (quantity of registers out of bounds) */
    fprintf(stdout, "Test #3: Asking for zero registers (expected exception number 03)\n");
    rc = modbus_read_registers(ctx, 0, 0, register_values);
    if (-1 == rc) {
        fprintf(stderr, "Reading registers failed (modbus protocol exception number = %d). %s\n", errno - MODBUS_ENOBASE, modbus_strerror(errno));
    }

    sleep(1);

    modbus_end(ctx);
    free(register_values);

    return 0;
}
