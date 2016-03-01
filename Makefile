
LDFLAGS=-L/usr/lib -lmodbus -pthread
INFLAGS=-I/usr/include/modbus/
CC=gcc

modbusmaster.x: modbusmaster.c
	$(CC) -o modbusmaster.x modbusmaster.c $(INFLAGS) $(LDFLAGS)

modbussim.x: modbussim.c
	$(CC) -o modbussim.x modbussim.c $(INFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f modbussim.x
	rm -f modbusmaster.x

