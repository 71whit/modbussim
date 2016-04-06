
LDFLAGS=-L/usr/lib -lmodbus -pthread -lm
INFLAGS=-I/usr/include/modbus/
OPTFLAGS=-lasound
CC=gcc

modbusmaster.x: modbusmaster.c
	$(CC) -o modbusmaster.x modbusmaster.c $(INFLAGS) $(LDFLAGS)

modbussim.x: modbussim.c
	$(CC) -o modbussim.x modbussim.c $(INFLAGS) $(LDFLAGS)

modbussim_sound.x: modbussim.c
	$(CC) -o modbussim.x modbussim.c -D SOUND $(INFLAGS) $(LDFLAGS) $(OPTFLAGS)

clean:
	rm -f *.o
	rm -f modbussim.x
	rm -f modbusmaster.x

