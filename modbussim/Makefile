
LDFLAGS=-L/usr/lib -lmodbus -pthread -lm
INFLAGS=-I/usr/include/modbus/
OPTFLAGS=-lasound
CC=gcc

modbuswriter.x: modbuswriter.c
	$(CC) -o modbuswriter.x modbuswriter.c $(INFLAGS) $(LDFLAGS)

modbusregisters.x: modbusregisters.c
	$(CC) -o modbusregisters.x modbusregisters.c $(INFLAGS) $(LDFLAGS)

modbusreader.x: modbusreader.c
	$(CC) -o modbusreader.x modbusreader.c $(INFLAGS) $(LDFLAGS)

modbusexceptions.x: modbusexceptions.c
	$(CC) -o modbusexceptions.x modbusexceptions.c $(INFLAGS) $(LDFLAGS)

modbussim.x: modbussim.c
	$(CC) -o modbussim.x modbussim.c $(INFLAGS) $(LDFLAGS)

modbussim_sound.x: modbussim.c
	$(CC) -o modbussim_sound.x modbussim.c -D SOUND $(INFLAGS) $(LDFLAGS) $(OPTFLAGS)

clean:
	rm -f *.o
	rm -f modbussim.x
	rm -f modbuswriter.x
	rm -f modbusreader.x
	rm -f modbusregisters.x

