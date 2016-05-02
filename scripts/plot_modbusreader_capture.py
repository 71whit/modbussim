
# This script plots the output from running 
# modbusreader.x

from pylab import *
from matplotlib import rc, rcParams

# make matrix from input file
matrix = genfromtxt('modbusreader_capture.txt')

# Slice out required columns.
a = matrix[:,1]
b = matrix[:,3]
c = matrix[:,5]
i = matrix[:,17]
j = matrix[:,19]

# plot each column
plot(a,label='register 0')
plot(b,label='register 1')
plot(c,label='register 2')
plot(i,label='register 8')
plot(j,label='register 9')

legend(loc='upper left', prop={'size':10})
xlabel('time')
ylabel('rpm')

show()
