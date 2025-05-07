# Define compiler and compiler flags
CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra -g -pedantic
LIBFLAGS := -lncurses -pthread

# Define source, object, and binary directories
SRCDIR := src
OBJDIR := obj
BINDIR := bin
LOGDIR := log

# Define source and object files
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Define the binary file
TARGET := $(BINDIR)/ticket

# Default target
all: $(TARGET)

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) -c $< -o $@

# Link object files to create the binary file
$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) $^ -o $@ $(LIBFLAGS)

# Clean up object and binary files
clean:
	rm -rf $(OBJDIR) $(BINDIR) $(LOGDIR)

run: all
	./$(TARGET)

# Define phony targets
.PHONY: all clean run
