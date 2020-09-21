TARGET = shadetabler

CC     = gcc
CFLAGS = -Wall -Werror -O0 -Wno-unused-function

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES  := $(wildcard $(SRCDIR)/exoquant/*.c) $(wildcard $(SRCDIR)/lodepng/*.c) $(wildcard $(SRCDIR)/cwalk/*.c) $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/exoquant/*.h) $(wildcard $(SRCDIR)/lodepng/*.h) $(wildcard $(SRCDIR)/cwalk/*.h) $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDES)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	@rm -rf $(OBJDIR) $(BINDIR)
