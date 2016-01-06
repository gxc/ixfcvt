#!/usr/bin/make -f

SHELL = /bin/bash
CC = gcc
CFLAGS = -O0 -g -std=c99 -Wall -Wextra -Wshadow -Wpedantic -Werror
CPPFLAGS = -I.
LDFLAGS = -L.
LDLIBS =
OBJS = ixfcvt.o parse_t.o parse_c.o parse_d.o util.o
PROG = ixfcvt

.PHONY : all
all : $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDLIBS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

.PHONY : clean
clean :
	-rm $(OBJS) $(PROG)
	-rm *?~
