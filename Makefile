#############################################################################
#
# Makefile for Raspberry Pi
#
#############################################################################

TARGET = nRFsniffer
LIBS = -lbcm2835
CC = gcc
# The recommended compiler flags for the Raspberry Pi
CCFLAGS = -Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

.PHONY: clean all default

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
#	-rm -f $(TARGET)
