CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc
SRCDIR = src
BUILDDIR= build
TARGET = fat16

SRCS = $(SRCDIR)/fat16_fs.c $(SRCDIR)/fat16_cli.c
OBJS = $(BUILDDIR)/fat16_fs.o $(BUILDDIR)/fat16_cli.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/fat16.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
