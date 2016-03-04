/* 
 * This code started as the 'bandwidth-server-many-up.c' example from 
 * https://github.com/stephane/libmodbus.git
 *
 * Modified and expanded by Whit Schonbein Spring 2016 
 *
 * One register is designated the 'rpm register'. If the -c option 
 * is used, then this register will increment with each update by the 
 * counter step (default 1). 
 *
 * If the -c option is not used, the rpm register attempts to 
 * adjust the actual rpm value to match the target rpm.
 * 
 * See the usage() for more info.
 *
 */

#ifndef __MODBUSSIM_H__
#define __MODBUSSIM_H__

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <modbus.h>

#define IP NULL
#define DEFAULT_PORT (1502)
#define NUM_CONNECTIONS_ALLOWED (1)
#define TIMEOUT (1) /* how long to wait for connections (seconds) */

/* modbus protocol addresses bits, input bits, registers, and input registers.
 * These parameters define the number of each in the simulation */
#define N_BITS (0)
#define N_IN_BITS (0)
#define N_IN_REGISTERS (0)

/* simulation parameters */
#define NS_PER_SEC (1000000000)
#define DEFAULT_UPDATE_FREQ (1000000000) /* default simulation update freq (nanoseconds) */
#define DEFAULT_TARGET_RPM (25000)   /* depending on rpm step, if this is too high rollover will mess things up */
#define DEFAULT_RPM_STEP (5)    /* this should probably be larger */

/* options */
typedef struct options_s {
    int num_registers;
    int rpm_register;
    int fail_threshold;
    int port;
    uint64_t update_frequency;
    int counter_step;
    uint16_t target_rpm;
} options_t;

/* global stuff */
static modbus_t *ctx = NULL; 
static modbus_mapping_t *mb_mapping;    /* modbus registers */
static int server_socket = -1;          /* server's socket */
static int halt_flag = 0;                
options_t options;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* the server thread function */
void *server( void *ptr );

/* the simulation thread function */
void *simulation( void *ptr );

/* process input options */
void get_options( int argc, char **argv, options_t *options);

/* display usage */
void usage();

/* print options */
void print_options( options_t *options );

#endif /* __MODBUSSIM_H__ */
