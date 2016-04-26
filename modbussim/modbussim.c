/* 
 * This code started as the 'bandwidth-server-many-up.c' example from 
 * https://github.com/stephane/libmodbus.git
 *
 * Modified and expanded by Whit Schonbein Spring 2016  
 *
 * Requires the libmodbus libraries available here:
 *  https://github.com/stephane/libmodbus.git
 *
 * To enable the audio sine wave output, compile with the -D SOUND flag.
 * Audio outout uses the alsa libraries, i.e., alsa-lib, alsa-utils, and 
 * compile with the -lasound library. 
 * 
 */

#include "modbussim.h"

void usage() {
    fprintf(stdout, "\nUsage:\n");
    fprintf(stdout, "   modbussim.x -R <int> [ -c <int> | -f <int> | -p <int> | -r <int> | -t <uint16_t> | -u <int> ]\n");
    fprintf(stdout, "\nwhere:\n");
    fprintf(stdout, "   -R <int> : number of registers (required)\n");
    fprintf(stdout, "   -c <int> : enable monotone counter on rpm register with step size <int>\n");
    fprintf(stdout, "   -f <int> : set fail rpm threshold to <int> (default = 0 (never fail))\n", DEFAULT_FAIL_THRESHOLD);
    fprintf(stdout, "   -p <int> : use port number <int> (default = %d)\n", DEFAULT_PORT);
    fprintf(stdout, "   -r <int> : set rpm register to register #<int> (default is random)\n");
    fprintf(stdout, "   -s <int> : set update step for rpm register (default = %d)\n", DEFAULT_UPDATE_STEP);
    fprintf(stdout, "   -t <uint16_t> : set target rpm to <uint_16> in simulation (default = %d)\n", DEFAULT_TARGET_RPM);
    fprintf(stdout, "   -u <uint> : set simulation update frequency to <uint> nanoseconds (default 1000000000, (1 sec)))\n");
    exit(-1);
}

void print_options( options_t *options ) {
    fprintf(stdout, "Using the following options:\n");
    fprintf(stdout, "   Number of registers:            %d\n", options->num_registers);
    fprintf(stdout, "   RPM Register:                   %d\n", options->rpm_register);
    fprintf(stdout, "   Target rpm:                     %u\n", options->target_rpm);
    fprintf(stdout, "   Fail Threshold (0 = no fail):   %d\n", options->fail_threshold);
    fprintf(stdout, "   Port:                           %d\n", options->port);
    fprintf(stdout, "   Update Frequency:               %d\n", options->update_frequency);
    fprintf(stdout, "   Update Step:                    %d\n", options->update_step);
    fprintf(stdout, "   Counter Step (0 = no counting): %d\n", options->counter_step);
}

/* Convert two continguious uint8_t elements into one uint16_t */
void two_uint8_to_uint16(uint8_t *input, uint16_t *output) {
    uint16_t a = input[0];
    a = a << 8;
    uint8_t b = input[1];
    *output = a | b;
}

void get_options( int argc, char **argv, options_t *options ) {
    extern char *optarg;
    extern int optind;
    int c;
    options->num_registers = -1;
    options->rpm_register = -1;
    options->fail_threshold = DEFAULT_FAIL_THRESHOLD;
    options->port = DEFAULT_PORT;
    options->update_frequency = DEFAULT_UPDATE_FREQ;
    options->counter_step = 0;
    options->target_rpm = (uint16_t)DEFAULT_TARGET_RPM;
    options->update_step = DEFAULT_UPDATE_STEP;

    while ((c = getopt(argc, argv, "c:f:hp:r:R:s:t:u:")) != -1 ) {
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
                options->update_frequency = strtoull(optarg, NULL, 10);
                break;
            case 's':
                options->update_step = atoi(optarg);
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
        fprintf(stderr, "invalid update frequency (%ld)\n", options->update_frequency);
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
    pthread_t server_thread, simulation_thread, sound_thread;
    int ret;

    /* in case we need it */
    srand(time(NULL));

    /* process command line options */
    get_options( argc, argv, &options );
    if (options.rpm_register == -1) {
        options.rpm_register = rand() % options.num_registers;
    }        

    print_options( &options );
 
    /* get the new communication context */
    ctx = modbus_new_tcp(IP, options.port);
    

#ifdef DEBUG
    modbus_set_debug(ctx, TRUE);
#endif

    // set as tcp slave
    if (modbus_set_slave(ctx, MODBUS_TCP_SLAVE) != 0) {
        fprintf(stderr, "Failed to set modbus tcp slave number: %\n", modbus_strerror(errno));
        return -1;
    }

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

#ifdef SOUND
    ret = pthread_create( &sound_thread, NULL, sound, NULL);
    if (ret) {
        perror("Failed to create sound thread");
        modbus_free(ctx);
        modbus_mapping_free(mb_mapping);
        return -1;
    }
#endif

    /* wait for whatever */
    pthread_join( server_thread, NULL );
    pthread_join( simulation_thread, NULL );
#ifdef SOUND
    pthread_join( sound_thread, NULL );
#endif

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
    
    /* get header length */
    int header_length = modbus_get_header_length(ctx);

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

                    /**************************************************************
                     * Explicit handling of modbus exception generation goes here 
                     **************************************************************/

                    /* Is the function code supported? */
                    if (query[header_length] > 0x2b) {
                        fprintf(stderr, "Connection attempted to use an invalid function code (%d)\n", query[header_length]);
                        /* should return exception code 01 */
                        pthread_mutex_lock( &lock );
                        modbus_reply_exception(ctx, query, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
                        pthread_mutex_unlock( &lock );
                        continue;
                    }

                    /* Function 0x03: read holding registers 
                       This is a partial implementation of the error logic given 
                       in figure 13 of the Modbus_Application_Protocol_V1 document */
                    if (query[header_length] = 0x03) {
                        /* get the requested register and the number of registers requested */
                        uint16_t start_reg;
                        uint8_t *uint = &query[header_length+1];
                        two_uint8_to_uint16(uint, &start_reg);
                        uint16_t num_reg;
                        uint = &query[header_length+3];
                        two_uint8_to_uint16(uint, &num_reg);

                        /* Is number of registers requested within bounds? */ 
                        if (num_reg < 1 || num_reg > MAX_NUM_REGISTERS) {
                            fprintf(stdout, "Function 0x03: Connection requested too few or too many registers (%d)\n", num_reg);
                            /* return exception code 03 */
                            pthread_mutex_lock( &lock );
                            modbus_reply_exception(ctx, query, MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE);
                            pthread_mutex_unlock( &lock );
                            continue;
                        }                    
    
                        /* are the offsets within bounds? */
                        if (start_reg < 0 || start_reg >= options.num_registers || start_reg+num_reg-1 > (options.num_registers-1)) {
                            fprintf(stderr, "Function 0x03: Connection requested registers out of bounds.\n");
                            /* return exception code 02 */
                            pthread_mutex_lock( &lock );
                            modbus_reply_exception(ctx, query, MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
                            pthread_mutex_unlock( &lock );
                            continue;
                        }
                    }

                    /* if no errors are raised, treat it like a normal request */
                    pthread_mutex_lock( &lock );
                    modbus_reply(ctx, query, rc, mb_mapping);
                    pthread_mutex_unlock( &lock );
                } else if (rc == -1) {
                    /* error or disconnect */
                    printf("Connection closed on socket %d\n", current_socket);
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
    pthread_mutex_lock( &lock );
    actual_rpm = options.target_rpm;
    pthread_mutex_unlock( &lock );
    
    /* for nanosleep */
    struct timespec update_freq;
    update_freq.tv_sec = options.update_frequency / NS_PER_SEC;
    update_freq.tv_nsec = options.update_frequency % NS_PER_SEC;

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

        nanosleep(&update_freq, NULL);

        /* if the counter step is activated, then it overrides the rpm simulation */
        if (options.counter_step > 0) {
            pthread_mutex_lock( &lock );
            mb_mapping->tab_registers[options.rpm_register] += options.counter_step;
            actual_rpm = mb_mapping->tab_registers[options.rpm_register];
            pthread_mutex_unlock( &lock );
        } else {
            /* deliberate race condition with the server thread */
            if (mb_mapping->tab_registers[options.rpm_register] < options.target_rpm )
                actual_rpm += options.update_step;
            else actual_rpm -= options.update_step; 
        }

        /* in either case, update all other registers by random increments.
         * rollover is ok */ 
        int i;
        for (i = 0; i < options.num_registers; i++ ) {
            if (i != options.rpm_register ) {
                mb_mapping->tab_registers[i] += ((rand() % 4096) - 2048);
                //printf("New register[%d] value = %u\n", i, mb_mapping->tab_registers[i]);
            } else {
                //printf("Actual RPM: %04X, RPM Register: %04X\n", actual_rpm, mb_mapping->tab_registers[options.rpm_register]);
                printf("Actual RPM: %d, RPM Register: %d\n", actual_rpm, mb_mapping->tab_registers[options.rpm_register]);
            }
        }

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

#ifdef SOUND

void *sound( void *ptr ) {

    static char *snd_device = "default";
    float snd_buffer[SND_BUFFER_LEN];

    int snd_err;
    int i;

    snd_pcm_t *snd_handle;
    snd_pcm_sframes_t snd_frames;

    snd_err = snd_pcm_open( &snd_handle, snd_device, SND_PCM_STREAM_PLAYBACK, 0);
    if (snd_err < 0) {
        fprintf(stderr, "Playback open error %s\n", snd_strerror(snd_err));
    }

    snd_err = snd_pcm_set_params( snd_handle, SND_PCM_FORMAT_FLOAT, SND_PCM_ACCESS_RW_INTERLEAVED, 1, SND_RATE, 1, 500000);
    if (snd_err < 0) {
        fprintf(stderr, "Set parameters error %s\n", snd_strerror(snd_err));
    }

    int current_freq = SND_LO_FREQ;
    float slope;
    if ( options.fail_threshold != 0 ) 
        slope = (SND_HI_FREQ - SND_LO_FREQ) / (float)(options.fail_threshold - options.target_rpm);
    else slope = (SND_HI_FREQ - SND_LO_FREQ) / (float)(USHRT_MAX - actual_rpm);

    for (;;) {
        
        if (halt_flag) {
            snd_pcm_close(snd_handle);
            return NULL;
        }

        /* get current frequency */
        pthread_mutex_lock( &lock );
        if (actual_rpm <= options.target_rpm) {
            current_freq = SND_LO_FREQ;
        } else {
            current_freq = (int)(SND_LO_FREQ + (slope * (actual_rpm - options.target_rpm)));
        }
        pthread_mutex_unlock( &lock );

        /* fill buffer */
        for (i = 0; i < SND_BUFFER_LEN; i++) {
            snd_buffer[i] = sin(2*M_PI*current_freq/SND_RATE*i);
        }

        /* play */
        snd_frames = snd_pcm_writei(snd_handle, snd_buffer, SND_BUFFER_LEN);

        if (snd_frames < 0)
            snd_frames = snd_pcm_recover(snd_handle, snd_frames, 0);
        if (snd_frames < 0) {
            fprintf(stderr, "snd_pcm_writei failed %s\n", snd_strerror(snd_frames));
            break;
        }

        if (snd_frames > 0 && snd_frames < SND_BUFFER_LEN)
            fprintf(stderr, "Short write (expected %li, wrote %li)\n", SND_BUFFER_LEN, snd_frames);
    }

    snd_pcm_close(snd_handle);
    pthread_mutex_lock( &lock );
    halt_flag = 1;
    pthread_mutex_unlock( &lock );

    return NULL;
}

#endif /* SOUND */
