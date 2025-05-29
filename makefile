CXX = g++

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

ifeq ($(PEXT), true)
CXXFLAGS += -DUSE_PEXT
endif

SRC = src/chess/*.cpp src/engine/*.cpp src/*.cpp

.PHONY: all blueberry clean makedir

all: blueberry

blueberry: makedir
	@$(CXX) $(CXXFLAGS) $(SRC) -o bin/blueberry.exe

clean: makedir
	@rm -rf bin
	@make makedir

makedir:
	@mkdir -p bin

.DEFAULT_GOAL := blueberry