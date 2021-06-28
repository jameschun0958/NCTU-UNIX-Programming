
CC	= gcc
CXX	= g++
CFLAGS	= -Wall -g 

PROGS	= hw4

all: $(PROGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $<

hw4: sdb.o ptools.o
	$(CXX) -o $@ $^ -lcapstone -g

%: %.o
	$(CC)  -o $@ $^ 

clean:
	rm -f *.o *~ $(PROGS)

