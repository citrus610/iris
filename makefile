CXX ?= g++
EXE ?= iris

NET ?= tulip
NET_FILE := $(NET).bin

CXXFLAGS += -DNNUE=\"$(NET_FILE)\"

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
	STATIC := -lstdc++fs -static -static-libgcc
else
	SUFFIX :=
	STATIC :=
endif

ifeq ($(PROF), true)
	CXXPROF += -pg -no-pie
else
	CXXPROF += -s
endif

ifeq ($(DEBUG), true)
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -std=c++20 -Wall -pthread -Og -g -no-pie
else
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -DNDEBUG -std=c++20 -Wall -O3 -pthread -flto $(CXXPROF)
endif

ifeq ($(TUNE), true)
	CXXFLAGS += -DTUNE
endif

ifeq ($(PEXT), true)
	CXXFLAGS += -DPEXT
endif

SRC := src/chess/*.cpp src/engine/*.cpp src/*.cpp
EXE := $(EXE)$(SUFFIX)

.PHONY: all build loadnet iris v1 v2 v3 release datagen cleannet clean

all: iris

build:
	@$(CXX) $(CXXFLAGS) -march=native $(SRC) $(STATIC) -o $(EXE)

iris: loadnet build cleannet

v1:
	@$(CXX) $(CXXFLAGS) -march=x86-64 $(SRC) $(STATIC) -o iris_x86-64-v1$(SUFFIX)

v2:
	@$(CXX) $(CXXFLAGS) -march=x86-64-v2 $(SRC) $(STATIC) -o iris_x86-64-v2$(SUFFIX)

v3:
	@$(CXX) $(CXXFLAGS) -march=x86-64-v3 -DPEXT $(SRC) $(STATIC) -o iris_x86-64-v3$(SUFFIX)

release: loadnet v1 v2 v3 cleannet

datagen: loadnet
	@mkdir -p bin
	@$(CXX) $(CXXFLAGS) -DDATAGEN -march=native $(SRC) $(STATIC) -o bin/datagen$(SUFFIX)
	@make cleannet

loadnet:
	@curl -sOL https://github.com/citrus610/iris-net/releases/download/$(NET)/$(NET_FILE);

cleannet:
	@rm -rf $(NET_FILE)

clean: cleannet
	@rm -rf $(EXE)

.DEFAULT_GOAL := iris