CC = gcc
CFLAGS = -O2 -Wall -Wextra -g -std=c99
LDFLAGS = -lm

TARGET = intfp_example
SRCS = main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c intfp.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
