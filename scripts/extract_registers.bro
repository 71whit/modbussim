#
# This script detects modbus 'read holding registers' communications 
#   (modbus protocol function 0x03, if I recall correctly).
#   The values of the registers as returned by the slave are logged.
#
# This script makes the following assumptions:
#   1. There is a single modbus connection in the packet stream.
#      (TODO: set up table for tracking register values by uid)
#   2. The relevant port MUST be set in the modbus protocol 
#      main.bro (bro/shared/protocols/modbus/main.bro). Otherwise 
#      there will be nothing to process.
#
# To generate graph of data: capture tcp packets -> run this script -> run extract_registers_transpose.py 
# on the resulting .log file -> plot using the plotting python script
#

@load base/protocols/modbus

module Modbus;

export {
    redef enum Log::ID += { Modbus::EXTRACT_REGISTERS_LOG };

    type RegLog: record {
        ## timestamp
        ts: time    &log;
        ## register 
        register:   count   &log;
        ## value
        value:  count   &log;
    };
}

# add an entry to the modbus record to store the start address
redef record Modbus::Info += {
    start_address: count &default=0;
};

# start log
event bro_init() &priority=5
{
    Log::create_stream(Modbus::EXTRACT_REGISTERS_LOG, [$columns=RegLog, $path="modbus_registers_extract"]);
}

# we need to process this event to get the start register
# when the response comes, we use this value to get the register offsets
event modbus_read_holding_registers_request(c: connection, headers: ModbusHeaders, start_address: count, quantity: count)
{
    # modbus indexing starts at 1
    c$modbus$start_address = start_address + 1;
}

# process the registers in the slave response
event modbus_read_holding_registers_response(c: connection, headers: ModbusHeaders, registers: ModbusRegisters)
{
    for ( i in registers )
    {
        local rec: RegLog = [$ts=network_time(), $register=c$modbus$start_address, $value=registers[i]];
        Log::write(EXTRACT_REGISTERS_LOG, rec);
        ++c$modbus$start_address;
    }
}

