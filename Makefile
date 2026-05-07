CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=

TARGET = ir_beacon_cordic
SRC = ir_beacon_cordic.c

BASE ?= 0xFF200000
THRESHOLD ?= 180
FOCAL_X ?= 500
FOCAL_Y ?= 500

.PHONY: all run continuous calibrate save clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	chmod +x $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET) --base $(BASE) --threshold $(THRESHOLD) --focal-x $(FOCAL_X) --focal-y $(FOCAL_Y)

continuous: $(TARGET)
	sudo ./$(TARGET) --base $(BASE) --threshold $(THRESHOLD) --focal-x $(FOCAL_X) --focal-y $(FOCAL_Y) --continuous

calibrate: $(TARGET)
	sudo ./$(TARGET) --base $(BASE) --focal-x $(FOCAL_X) --focal-y $(FOCAL_Y)

save: $(TARGET)
	sudo ./$(TARGET) --base $(BASE) --threshold $(THRESHOLD) --save-pgm capture.pgm

clean:
	rm -f $(TARGET) capture.pgm

