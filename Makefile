APPNAME:=$(shell basename `pwd`)
CC=cc
CXX=c++
COMPILER=$(CC)

LDFLAGS = -L./lib
LDLIBS = -lraylibdll -lode_doubled

CFLAGS:= -Iode-0.16.4/include/ode -Iode-0.16.4/include -Iinclude/raylib -Iinclude -I./
CFLAGS+= -std=c99 -DPLATFORM_DESKTOP

SRC:=$(wildcard src/*.c)
OBJ:=$(SRC:src/%.c=build/%.o)
INC:=$(wildcard include/*.h)

#if gcc
ifeq ($(filter gcc g++, $(CC) $(CXX)), gcc g++)
    LDLIBS += -pthread -lstdc++
endif

# set to release or debug
all: release

$(APPNAME): $(OBJ)
	$(COMPILER) $(OBJ) -o $(APPNAME) $(LDFLAGS) $(LDLIBS)

$(OBJ): build/%.o : src/%.c
	$(COMPILER) $(CFLAGS) -c $< -o $@

.PHONY: debug release inst

debug: CFLAGS+= -g
debug:
	@echo "*** made DEBUG target ***"

release:
	@echo "*** made RELEASE target ***"

release: CFLAGS+= -Ofast

debug release inst: clean $(APPNAME)

.PHONY:	clean
clean:
	rm build/* -rf
	rm $(APPNAME) -f

style: $(SRC) $(INC)
	astyle -A10 -s4 -S -p -xg -j -z2 -n src/* include/*
