#!/usr/bin/make -f

# Copyright 2016 Guo, Xingchun <guoxingchun@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wshadow -pedantic -Werror
CPPFLAGS = -I. -D_XOPEN_SOURCE=600 -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS = -L.
LDLIBS =
OBJS = main.o ixfcvt.o summary.o parse_t.o parse_c.o parse_d.o tc2sql.o d2sql.o util.o
PROG = ixfcvt

.PHONY : all

.PHONY : release
release : CFLAGS += -O2
release : CPPFLAGS += -DNDEBUG
release : LDFLAGS += -s
release : all

.PHONY : debug
debug : CFLAGS += -Og -g
debug : CPPFLAGS += -DDEBUG
debug : all

.PHONY : prof
prof : CFLAGS += -O2 -g
prof : LDFLAGS += -DNDEBUG
prof : all

all : $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDLIBS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

.PHONY : clean
clean :
	-rm $(OBJS) $(PROG)
	-rm *~

.PHONY : cleanall
cleanall : clean
	-rm *.out.*
