CXX ?= g++
EXE ?= iris

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
else
	SUFFIX :=
endif

ifeq ($(PROF), true)
	CXXPROF += -pg -no-pie
else
	CXXPROF += -s
endif

ifeq ($(BUILD), debug)
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -std=c++20 -Wall -Og -g -no-pie
else
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -DNDEBUG -std=c++20 -Wall -O3 -flto $(CXXPROF) -march=native
endif

ifeq ($(TUNE), true)
	CXXFLAGS += -DTUNE
endif

ifeq ($(PEXT), true)
	CXXFLAGS += -DUSE_PEXT
endif

SRC := src/chess/*.cpp src/engine/*.cpp src/*.cpp
EXE := $(EXE)$(SUFFIX)

.PHONY: all iris clean

all: iris

iris:
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE)

clean:
	@rm -rf $(EXE)

.DEFAULT_GOAL := iris