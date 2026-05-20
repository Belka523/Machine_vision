# Makefile для Windows (MinGW/GCC)
#
# Команды в терминале:
#   mingw32-make — собрать
#   mingw32-make clean — удалить скомпилированные файлы
#


CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm          

TARGET = imgproc.exe   
SRCS   = main.c imgproc.c
OBJS   = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


%.o: %.c imgproc.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del /f $(TARGET) $(OBJS) 2>nul || rm -f $(TARGET) $(OBJS)

.PHONY: all clean
