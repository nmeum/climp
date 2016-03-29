# See LICENSE for license details.
BIN_NAME = climp

OBJECTS = $(BIN_NAME).o scanner.o
HEADERS = climp.h scanner.h

CFLAGS ?= -O0 -g -pedantic -Wall
CFLAGS += -std=c99 -I./compat

CC      ?= cc
LDFLAGS ?=

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(BIN_NAME)
$(BIN_NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(BIN_NAME) $(OBJECTS)
