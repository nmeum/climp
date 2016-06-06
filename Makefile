# See LICENSE for license details.
BIN_NAME = climp

OBJECTS = $(BIN_NAME).o scanner.o parser.o util.o
HEADERS = util.h scanner.h parser.h

CFLAGS ?= -O0 -g -pedantic -Wall -Werror
CFLAGS += -pthread -std=c99 -D_POSIX_C_SOURCE=200809L -I./compat

CC      ?= cc
LDFLAGS ?=

%.o: %.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(BIN_NAME)
$(BIN_NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(BIN_NAME) $(OBJECTS)
