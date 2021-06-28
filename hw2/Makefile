CC = g++
CFLAG = -fPIC -shared -g -Wall
MAIN = logger
SAMPLE = sample
INJ_LIB = logger.so

all: $(MAIN) $(INJ_LIB)

$(MAIN):hw2.cpp
	$(CC) $^ -o $(MAIN)

%.so:logger.cpp
	$(CC) $^ $(CFLAG) -o $(INJ_LIB) -ldl

test:
	$(CC) sample.cpp -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -o $(SAMPLE)
	LD_PRELOAD=./$(INJ_LIB) ./$(MAIN) ./$(SAMPLE) || true

clean:
	rm -f $(MAIN) $(INJ_LIB) $(SAMPLE)
