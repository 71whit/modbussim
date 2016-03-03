/* 
 * This code started as the 'bandwidth-server-many-up.c' example from 
 * https://github.com/stephane/libmodbus.git
 *
 * Modified and expanded by Whit Schonbein Spring 2016  
 *
 */

#include "modbussim.h"

void usage() {
    fprintf(stdout, "\nUsage:\n");
    fprintf(stdout, "   modbussim.x -R <int> [ -c <int> | -f <int> | -p <int> | -r <int> | -t <uint16_t> | -u <int> ]\n");
    fprintf(stdout, "\nwhere:\n");
    fprintf(stdout, "   -R <int> : number of registers (required)\n");
    fprintf(stdout, "   -c <int> : enable monotone counter on rpm register with step size <int>\n");
    fprintf(stdout, "   -f <int> : set fail threshold to <int>\n");
    fprintf(stdout, "   -p <int> : use port number <int>\n");
    fprintf(stdout, "   -r <int> : set rpm register to <int> (default is random)\n");
    fprintf(stdout, "   -t <uint16_t> : set target rpm to <int> in simulation\n");
    fprintf(stdout, "   -u <int> : set simulation update frequency to <int> seconds (default 1)\n");
    exit(-1);
}

void print_options( options_t *options ) {
    fprintf(stdout, "Using the following options:\n");
    fprintf(stdout, "   Number of registers:            %d\n", options->num_registers);
    fprintf(stdout, "   RPM Register:                   %d\n", options->rpm_register);
    fprintf(stdout, "   Fail Threshold (0 = no fail):   %d\n", options->fail_threshold);
    fprintf(stdout, "   Port:                           %d\n", options->port);
    fprintf(stdout, "   Update Frequency:               %d\n", options->update_frequency);
    fprintf(stdout, "   Counter Step (0 = no counting): %d\n", options->counter_step);
    fprintf(stdout, "   Target rpm:                     %u\n", options->target_rpm);
}

void get_options( int argc, char **argv, options_t *options ) {
    extern char *optarg;
    extern int optind;
    int c;
    options->num_registers = -1;
    options->rpm_register = -1;
    options->fail_threshold = 0;
    options->port = DEFAULT_PORT;
    options->update_frequency = DEFAULT_UPDATE_FREQ;
    options->counter_step = 0;
    options->target_rpm = (uint16_t)DEFAULT_TARGET_RPM;

    while ((c = getopt(argc, argv, "c:f:hp:r:R:t:u:")) != -1 ) {
        switch (c) {
            case 'c':
                options->counter_step = atoi(optarg);
                break;
            case 'f':
                options->fail_threshold = atoi(optarg);
                break;
            case 'h':
                usage();
            case 'p':
                options->port = atoi(optarg);
                break;
            case 'r':
                options->rpm_register = atoi(optarg);
                break;
            case 'R':
                options->num_registers = atoi(optarg);
                break;
            case 't':
                options->target_rpm = atoi(optarg);
                break;
            case 'u':
                options->update_frequency = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Unknown option: -%c\n", c);
                break;
        }
    } /*while*/

    if (options->num_registers <= 0) {
        fprintf(stderr, "-R <int> (number of registers) is either invalid or missing\n");
        usage();
    }

    if (options->counter_step < 0) {
        fprintf(stderr, "invalid value (%d) for counter step (-c)\n", options->counter_step);
        usage();
    }

    if (options->fail_threshold < 0) {
        fprintf(stderr, "invalid value (%d) for fail threshold (-f)\n", options->fail_threshold);
        usage();
    }

    if (options->rpm_register > options->num_registers-1) {
        fprintf(stderr, "rpm register out of bounds (register %d for %d total registers)\n", options->rpm_register, options->num_registers);
        usage();
    }

    if (options->update_frequency < 1) {
        fprintf(stderr, "invalid update frequency (%d)\n", options->update_frequency);
        usage();
    }

    if (options->target_rpm < 0) {
        fprintf(stderr, "invalid target rpm (%d)\n", options->target_rpm);
        usage();
    }

    if (options->target_rpm > options->fail_threshold && options->fail_threshold != 0) {
        fprintf(stderr, "WARNING: target rpm for simulation is greater than fail threshold (%u > %d)\n", options->target_rpm, options->fail_threshold);
    }
}

int main(int argc, char **argv) {
    
    /* for threads */
    pthread_t server_thread, simulation_thread;
    int ret;

    /* in case we need it */
    srand(time(NULL));

    /* process command line options */
    get_options( argc, argv, &options );
    if (options.rpm_register == -1) {
        options.rpm_register = rand() % options.num_registers;
    }        

    print_options( &options );
 
    // get the new communication context
    ctx = modbus_new_tcp(IP, options.port);

#ifdef DEBUG
    modbus_set_debug(ctx, TRUE);
#endif
   
    /* allocate the registers etc. */
    mb_mapping = modbus_mapping_new(N_BITS, N_IN_BITS, options.num_registers, N_IN_REGISTERS);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate new modbus mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    /* initialize registers to random values */
    int i;
    for (i = 0; i < options.num_registers; i++ ) {
        mb_mapping->tab_registers[i] = (rand() % 65536) - 32768;
    }
    
    /* split threads */
    ret = pthread_create( &server_thread, NULL, server, NULL);
    if (ret) {
        perror("Failed to create server thread");
        modbus_free(ctx);
        modbus_mapping_free(mb_mapping);
        return -1;
    }

    ret = pthread_create( &simulation_thread, NULL, simulation, NULL);
    if (ret) {
        perror("Failed to create server thread");
        modbus_free(ctx);
        modbus_mapping_free(mb_mapping);
        return -1;
    }

    /* wait for whatever */
    pthread_join( server_thread, NULL );
    pthread_join( simulation_thread, NULL );

    /* clean up and exit */

    return 0;
}

void *server( void * ptr ) {
    
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    int rc;
    fd_set refset;
    fd_set tmpset;
    int fdmax;
    int current_socket;

    /* check for halting state after timeout */
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    // get the server socket
    server_socket = modbus_tcp_listen(ctx, NUM_CONNECTIONS_ALLOWED);

    /* Clear the reference set and add the server socket */
    FD_ZERO(&refset);
    FD_SET(server_socket, &refset);

    /* keep track of maximum fd */
    fdmax = server_socket;

    int count = 0;
    /* loop forever accepting connections */
    for (;;) {

        if ( halt_flag ) return NULL;

        /* monitor the fds in the refset. */
        tmpset = refset;
        while (select(fdmax+1, &tmpset, NULL, NULL, &timeout) == -1) {
            if (halt_flag) return NULL;
        }

        /* Run through the existing connections looking for data to be
         * read */
        for (current_socket = 0; current_socket <= fdmax; current_socket++) {

            /* if the current socket is not set in the tmpset, then skip */
            if (!FD_ISSET(current_socket, &tmpset)) {
                continue;
            }

            /* if the current socket is the master socket, then a new connection 
               has come in on the master. create a new socket for it and 
               transfer control */
            if (current_socket == server_socket) {
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                /* create new socket/fd for the connection */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1) {
                    perror("Server accept() error");
                } else {
                    FD_SET(newfd, &refset);

                    if (newfd > fdmax) {
                        /* Keep track of the maximum */
                        fdmax = newfd;
                    }
                    //printf("New connection from %s:%d on socket %d\n",
                    //       inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
                }
            /* Else, the current socket is an existing connection; try to 
               interact with it; if cannot, close it. */
            } else {
                modbus_set_socket(ctx, current_socket);
                rc = modbus_receive(ctx, query);
                if (rc > 0) {
                    pthread_mutex_lock( &lock );
                    modbus_reply(ctx, query, rc, mb_mapping);
                    pthread_mutex_unlock( &lock );
                } else if (rc == -1) {
                    /* error or disconnect */
                    //printf("Connection closed on socket %d\n", current_socket);
                    close(current_socket);

                    /* Remove from reference set */
                    FD_CLR(current_socket, &refset);

                    /* decrement the max fd if this was the max */
                    if (current_socket == fdmax) {
                        fdmax--;
                    }
                }
            }
        }
    }

    return NULL;
}

void *simulation( void * ptr ) {
    
    uint16_t counter = 0x0;

    /* start the simulation with the rpms at the target */
    uint16_t actual_rpm = options.target_rpm;

    for(;;) {
            
        /* we need a gap between getting the sensor data into the 
         * register and using it to control rpm, because otherwise it is 
         * difficult to fool the simulation
         * TODO: play around with this a bit?
         */
        if (!(options.counter_step > 0)) {
            pthread_mutex_lock( &lock );
            mb_mapping->tab_registers[options.rpm_register] = actual_rpm;
            pthread_mutex_unlock( &lock );
        }

        sleep(options.update_frequency);

        /* if the counter step is activated, then it overrides the rpm simulation */
        if (options.counter_step > 0) {
            pthread_mutex_lock( &lock );
            mb_mapping->tab_registers[options.rpm_register] += options.counter_step;
            actual_rpm = mb_mapping->tab_registers[options.rpm_register];
            pthread_mutex_unlock( &lock );
        } else {
            /* deliberate race condition with the server thread */
            if (mb_mapping->tab_registers[options.rpm_register] < options.target_rpm )
                actual_rpm += DEFAULT_RPM_STEP;
            else actual_rpm -= DEFAULT_RPM_STEP; 
        }

        /* in either case, update all other registers by random increments */
        int i;
        for (i = 0; i < options.num_registers; i++ ) {
            if (i != options.rpm_register ) {
                mb_mapping->tab_registers[i] += ((rand() % 4096) - 1024);
                //printf("New register[%d] value = %u\n", i, mb_mapping->tab_registers[i]);
            } else {
                printf("Actual RPM: %04X, RPM Register: %04X\n", actual_rpm, mb_mapping->tab_registers[options.rpm_register]);
            }
        }

        //printf("- - - - - - - - - - - - - - - - - - - - - - -\n");

        if (options.fail_threshold > 0 ) {
            if (actual_rpm > options.fail_threshold) {
                printf("Exploding.\n");
                break;
            }
        }
    } /* end for */

    pthread_mutex_lock( &lock );
    halt_flag = 1;
    pthread_mutex_unlock( &lock );

    return NULL;
}
