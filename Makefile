# Makefile for Arduino LED blink project (using .ino)
BOARD      = arduino:avr:uno
PORT       = /dev/ttyACM1
SKETCH_DIR = build_sketch
SKETCH_NAME = sonar_main

# The source .ino file
# Allow overriding sketch name: make SKETCH_NAME=sonar2 upload
SKETCH_NAME   ?= sonar_main
SRC_INO       = src/$(SKETCH_NAME).ino

# Target folder for compilation (must match sketch name)
SKETCH_FOLDER = $(SKETCH_DIR)/$(SKETCH_NAME)
SKETCH_FILE   = $(SKETCH_FOLDER)/$(SKETCH_NAME).ino

all: upload

$(SKETCH_FILE): $(SRC_INO)
	mkdir -p $(SKETCH_FOLDER)
	cp $(SRC_INO) $(SKETCH_FILE)

compile: $(SKETCH_FILE)
	arduino-cli compile --fqbn $(BOARD) $(SKETCH_FOLDER)

upload: compile
	arduino-cli upload --port $(PORT) --fqbn $(BOARD) $(SKETCH_FOLDER)

upload2: SKETCH_NAME = sonar2
upload2: upload
	
compile2: SKETCH_NAME = sonar2
compile2: compile

clean:
	rm -rf $(SKETCH_DIR)

.PHONY: all compile upload clean
