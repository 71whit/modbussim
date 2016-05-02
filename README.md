# modbussim

`modbussim` is a simple modbus-based PLC simulation. It was developed to explore the basics of 
exploiting the modbus protocol for purposes of data gathering and/or modifying the behavior of 
industrial systems.

This project was developed for educational purposes: CS544 Cybersecurity, University of New 
Mexico, Spring 2016.

Authors: **Whit Schonbein** (schonbein [at] cs.unm.edu) and **Evan Dye** (etdye [at] cs.unm.edu).

## Components

*modbussim*: the simulation. compliation requires the *libmodbus* library at 
[libmodbus.org](http://libmodbus.org/). Compilation with the optional annoying-sine-wave-indicating-imminent-explosion 
thread requires the [alsa sound libraries](http://www.alsa-project.org/main/index.php/Main_Page). The 
*modbussim* directory also contains some tools for writing to and reading from the simulation, as well as 
determining how many registers an instance of the simulation provides.

*modbuster*: tool for exploiting the simulaton for purposes of wreaking havoc. `modbuster` determines 
the relevant port and register for causing the simulation to go into an 'explode' state, and then puts it there.

*scripts*: scripts (bro, python) we used to process and plot captured modbus over tcp packets.
