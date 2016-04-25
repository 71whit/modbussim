LFLAGS=-lmodbus
IFLAGS=-I/usr/include/modbus/
CC=gcc

options.o: options.c options.h
	$(CC) $(IFLAGS) -c options.c -o options.o

main.o: main.c options.h
	$(CC) $(IFLAGS) -c main.c -o main.o

modbuster.exe: main.o options.o
	$(CC) main.o options.o $(LFLAGS) -o modbuster.exe

all: modbuster.exe

clean:
	rm *.o *.exe

