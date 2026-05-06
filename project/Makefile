CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDLIBS ?= -pthread

TARGET = ov7670_sccb_probe
SRC = ov7670_sccb_probe.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET) $(RUN_ARGS)

clean:
	rm -f $(TARGET)
