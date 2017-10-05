CC := gcc
ARMCC := arm-linux-gcc -std=c11
SRCEXT := c

SRCDIR := src
BUILDDIR := build
TARGET := xbox
# find all the sources in $SRC
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))

#remap .o to build folder
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

CFLAGS := -g # -Wall
LIB := -lusb-1.0 -lpthread -lncurses
INC := -I include


all: $(TARGET)

run: TERM = linux
run: $(TARGET)
	@./$(TARGET)

target: CC = $(ARMCC)
target: clean all

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@echo " Building..."
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $< $(CFLAGS) $(INC) -c -o $@"; $(CC) $< $(CFLAGS) $(INC) -c -o $@

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)
.PHONY: clean
