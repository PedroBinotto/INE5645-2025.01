CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -pedantic
LIBFLAGS := -lncurses -pthread

APPNAME := restaurant

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
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) $^ -o $@ $(LIBFLAGS)

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
	./$(TARGET) $(ARGS)
