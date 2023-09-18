APPNAME:=rlOde.exe
CC=gcc
CXX=g++
COMPILER=$(CXX)

LDFLAGS = -Llib
LDLIBS := -l:raylib.dll -l:ode_double.dll -lwinmm

CFLAGS:= -Iode/include/ode -Iode/include -Iinclude/raylib -Iinclude -I./
CFLAGS+= -std=c++20 -DPLATFORM_DESKTOP

SRC:=$(wildcard src/*.cpp)
OBJ:=$(SRC:src/%.cpp=build/%.obj)
INC:=$(wildcard include/*.h)
INC+=$(wildcard ode/include/ode/*.h)

# set to release or debug
all: release

$(APPNAME): $(OBJ)
	$(COMPILER) $(OBJ) -o $(APPNAME) $(LDFLAGS) $(LDLIBS)

$(OBJ): build/%.obj : src/%.cpp
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
