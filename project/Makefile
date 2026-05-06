CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?= -pthread
TARGET = ov7670_sccb_probe
SRC = probe.c
.PHONY: all run run-tmp debug-xclk clean
all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	chmod +x $(TARGET)
run: $(TARGET)
	sudo ./$(TARGET)
run-tmp: $(TARGET)
	cp $(TARGET) /tmp/$(TARGET)
	chmod +x /tmp/$(TARGET)
	sudo /tmp/$(TARGET)
debug-xclk: $(TARGET)
	sudo ./$(TARGET) --gpio-xclk
clean:
	rm -f $(TARGET)
