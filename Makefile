APPNAME:=rlOde.exe
CC=gcc
CXX=g++
COMPILER=$(CXX)

LDFLAGS = -L./lib
LDLIBS := -l:raylib.dll -l:ode_double.dll -lopengl32 -lgdi32 -lwinmm

INCLUDEPATHS:= -Iode/include/ode -Iode/include -Iinclude/raylib -Iinclude -I./
CFLAGS:= -std=c++20 -DPLATFORM_DESKTOP

SRC:=$(wildcard src/*.cpp)
OBJ:=$(SRC:src/%.cpp=build/%.obj)

# set to release or debug
all: release

$(APPNAME): $(OBJ)
	$(COMPILER) $(OBJ) -o build/$(APPNAME) $(LDFLAGS) $(LDLIBS)

$(OBJ): build/%.obj : src/%.cpp
	$(COMPILER) $(INCLUDEPATHS) $(CFLAGS) -c $< -o $@

.PHONY: debug release inst

debug: CFLAGS+= -g
debug:
	@echo "*** made DEBUG target ***"

release:
	@echo "*** made RELEASE target ***"

release: CFLAGS+= -Ofast

debug release: clean $(APPNAME)

.PHONY:	clean
clean:
	rm build/$(APPNAME) -f

style: $(SRC) $(INC)
	astyle -A10 -s4 -S -p -xg -j -z2 -n src/* include/*
