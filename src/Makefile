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
CFLAGS = -std=c99 -D_XOPEN_SOURCE=600 -Wall -Wextra -Wshadow -Wpedantic -Werror
CFLAGS_DEBUG = -O0 -g -DDEBUG
CFLAGS_RELEASE = -O2 -DNDEBUG
CPPFLAGS = -I.
LDFLAGS = -L.
LDLIBS =
OBJS = main.o ixfcvt.o summary.o parse_t.o parse_c.o parse_d.o tc_rec2sql.o d_rec2sql.o util.o
PROG = ixfcvt

.PHONY : all

.PHONY : release
release : CFLAGS += $(CFLAGS_RELEASE)
release : all

.PHONY : debug
debug : CFLAGS += $(CFLAGS_DEBUG)
debug : all

all : $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDLIBS)

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

.PHONY : clean
clean :
	-rm $(OBJS) $(PROG)
	-rm *~

