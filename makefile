CXX ?= g++
EXE ?= iris

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

ifeq ($(BUILD), debug)
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -std=c++20 -Wall -Og -g -no-pie
else
	CXXFLAGS += -fdiagnostics-color=always -DUNICODE -DNDEBUG -std=c++20 -Wall -O3 -flto $(CXXPROF)
endif

ifeq ($(TUNE), true)
	CXXFLAGS += -DTUNE
endif

ifeq ($(PEXT), true)
	CXXFLAGS += -DUSE_PEXT
endif

SRC := src/chess/*.cpp src/engine/*.cpp src/*.cpp
EXE := $(EXE)$(SUFFIX)

.PHONY: all iris v1 v2 v3 v4 release clean

all: iris

iris:
	@$(CXX) $(CXXFLAGS) -march=native $(SRC) $(STATIC) -o $(EXE)

v1:
	@$(CXX) $(CXXFLAGS) -march=x86-64 $(SRC) $(STATIC) -o iris_x86-64-v1$(SUFFIX)

v2:
	@$(CXX) $(CXXFLAGS) -march=x86-64-v2 $(SRC) $(STATIC) -o iris_x86-64-v2$(SUFFIX)

v3:
	@$(CXX) $(CXXFLAGS) -march=x86-64-v3 -DUSE_PEXT $(SRC) $(STATIC) -o iris_x86-64-v3$(SUFFIX)

v4:
	@$(CXX) $(CXXFLAGS) -march=x86-64-v4 -DUSE_PEXT $(SRC) $(STATIC) -o iris_x86-64-v4$(SUFFIX)

release: v1 v2 v3 v4

clean:
	@rm -rf $(EXE)

.DEFAULT_GOAL := iris