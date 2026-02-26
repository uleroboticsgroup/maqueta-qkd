CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -g -I$(CJSONDIR)
LDFLAGS = -lcurl

SRCDIR = .
CJSONDIR = ./cJSON
SRC = $(SRCDIR)/conections.c $(SRCDIR)/handle_json.c $(CJSONDIR)/cJSON.c $(SRCDIR)/cryptografy.c
OBJS = $(SRC:.c=.o)
TARGET = conections_app

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)