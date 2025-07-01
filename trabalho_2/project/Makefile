# **********************************************************************************************************************
# "USER-LEVEL" CONFIGURATION SECTION BEGINS HERE! THESE VARIABLES ARE SAFE TO TWEAK FOR DIFFERENT BEHAVIOUR

# Determines how many intances of the process will be spawned by MPICC
N_PROCS := 4

# "USER-LEVEL" CONFIGURATION SECTION ENDS HERE! DO NOT EDIT CODE BEYOUND THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING
# **********************************************************************************************************************

MPICC=mpic++
MPIR=mpirun
CXXFLAGS := -std=c++20 -Wall -Wextra -g -pedantic
LIBFLAGS :=

APPNAME := distributed

SRCDIR := src
OBJDIR := obj
BINDIR := bin
LOGDIR := log
TARGET := $(BINDIR)/$(APPNAME)

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: help

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(MPICC) $(CXXFLAGS) $(LIBFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(MPICC) $(CXXFLAGS) $(LIBFLAGS) $^ -o $@ $(LIBFLAGS)

.PHONY: help
help: Makefile
	@echo
	@echo " Choose a make command to run"
	@echo
	@sed -n 's/^##//p' $< | column -t -s ':' |  sed -e 's/^/ /'
	@echo

## build: compile project to binary
.PHONY: build
build: $(TARGET)

## clean: clean up object and binary files
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR) $(LOGDIR)

## run: build and run project;
## run $(ARGS): run with command line args via `make run ARGS="<arg1, arg2 ...>"`
.PHONY: run
run: build
	$(MPIR) -n $(N_PROCS) $(TARGET) $(ARGS)
