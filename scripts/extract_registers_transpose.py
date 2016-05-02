#!/usr/bin/python

# 2016 Whit Schonbein
#
# University of New Mexico
# Course: cs544
#
# Toolchain: capture tcp packets -> run bro script -> run this script -> plot
#
# used to transpose the data acquired by the 
# BRO script for extracting modbus registers 
# so that it can be graphed.

from __future__ import print_function
import sys

num_columns = 3 # this is determined by what the bro script outputs

if len(sys.argv) != 3:
  print('usage: ' + str(sys.argv[0]) + ' <infile>' + ' <number of registers>')
  exit()

num_registers = int(sys.argv[2])
infile = open(sys.argv[1])
count = 1
for line in infile:
    values = line.split()
    if (not values[0][0] == '#'):
        for col in range(0, num_columns):
            if (col == 0 and count == 1):
                print(values[col] + "\t",end='')
            elif (not (col == 0)):
                print(values[col] + "\t",end='')
        if (count == num_registers):
            print("")
            count = 1
        else:
            count = count + 1

infile.close()

