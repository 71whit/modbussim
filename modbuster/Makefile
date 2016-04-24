LFLAGS=-lmodbus
IFLAGS=-I/usr/include/modbus/
CC=gcc

options.o: options.c options.h
	$(CC) $(IFLAGS) -c options.c -o options.o

main.o: main.c options.h
	$(CC) $(IFLAGS) -c main.c -o main.o

attack_modbussim.exe: main.o options.o
	$(CC) main.o options.o $(LFLAGS) -o attack_modbussim.exe

all: attack_modbussim.exe

clean:
	rm *.o *.exe

