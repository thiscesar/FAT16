CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Isrc
SRCDIR  = src
BUILDDIR= build
TARGET  = $(BUILDDIR)/fat16   # <— binário final dentro de build/

SRCS = $(SRCDIR)/fat16_fs.c $(SRCDIR)/fat16_cli.c
OBJS = $(BUILDDIR)/fat16_fs.o $(BUILDDIR)/fat16_cli.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/fat16.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

# atalho opcional
IMG ?= ./imgs/disco1.img
run: all
	$(TARGET) $(IMG)
