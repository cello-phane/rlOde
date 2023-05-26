
APPNAME:=$(shell basename `pwd`)

INSTR:= -fsanitize=address,leak,undefined,pointer-compare,pointer-subtract
INSTR+= -fno-omit-frame-pointer --enable-ou --enable-libccd --with-box-cylinder=libccd --with-drawstuff=none --disable-demos --with-libccd=internal

LDFLAGS:=-L. -lm -pthread
LDFLAGS+=-lraylib -lode_double -lstdc++

CFLAGS:= -Wfatal-errors -pedantic -Wall -Wextra -Werror -I ../ode-0.16.3/include/ode -I ../ode-0.16.3/include -I ./include
CFLAGS+= -std=c99 -DPLATFORM_DESKTOP

SRC:=$(wildcard src/*.c)
OBJ:=$(SRC:src/%.c=build/%.o)
INC:=$(wildcard include/*.h)

CC=gcc

all: release

$(APPNAME): $(OBJ)
	$(CC) $(OBJ) -o $(APPNAME) $(LDFLAGS)

$(OBJ): build/%.o : src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: debug release inst

debug: CFLAGS+= -g
debug:
	@echo "*** made DEBUG target ***"

release: CFLAGS+= -O3
release:
	@echo "*** made RELEASE target ***"

inst: CFLAGS+= -g $(INSTR)
inst: LDFLAGS+= $(INSTR)
inst:
	@echo "*** made INSTRUMENTATION target ***"

release: CFLAGS+= -Ofast

debug release inst: clean $(APPNAME)

.PHONY:	clean
clean:
	rm build/* -f
	rm $(APPNAME) -f

style: $(SRC) $(INC)
	astyle -A10 -s4 -S -p -xg -j -z2 -n src/* include/*
