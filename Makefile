CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -Wno-sign-conversion
LDFLAGS = -lm

TARGET = spreadsheet
SOURCES = main.c spreadsheet.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.c spreadsheet.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q $(OBJECTS) $(TARGET).exe 2>nul || rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	.\$(TARGET).exe -i input.csv -o output.csv

test: $(TARGET)
	echo =A2,=A1 > cycle.csv
	.\$(TARGET).exe -i cycle.csv

.PHONY: all clean run test