# Makefile for Sonar Project (3 active sketches)
# Active sketches: sonar3 (default), direction_v2_recommended, sonar3_clean
# Archived sketches moved to src/archive/

BOARD      = arduino:avr:uno
PORT       = /dev/ttyACM0
SKETCH_DIR = build_sketch
SKETCH_NAME ?= sonar3

SRC_INO    = src/$(SKETCH_NAME).ino
SKETCH_FOLDER = $(SKETCH_DIR)/$(SKETCH_NAME)
SKETCH_FILE   = $(SKETCH_FOLDER)/$(SKETCH_NAME).ino

# --- Default target ---
all: upload

$(SKETCH_FILE): $(SRC_INO)
	mkdir -p $(SKETCH_FOLDER)
	cp $(SRC_INO) $(SKETCH_FILE)

compile: $(SKETCH_FILE)
	arduino-cli compile --fqbn $(BOARD) $(SKETCH_FOLDER)

upload: compile
	arduino-cli upload --port $(PORT) --fqbn $(BOARD) $(SKETCH_FOLDER)

# --- sonar3 (default product sketch) ---
.PHONY: upload_sonar3 compile_sonar3
upload_sonar3: SKETCH_NAME = sonar3
upload_sonar3: upload

compile_sonar3: SKETCH_NAME = sonar3
compile_sonar3: compile

# --- direction_v2_recommended (direction test with Kalman filter) ---
.PHONY: upload_direction compile_direction
upload_direction: SKETCH_NAME = direction_v2_recommended
upload_direction: upload

compile_direction: SKETCH_NAME = direction_v2_recommended
compile_direction: compile

# --- sonar3_clean (cleaned up version) ---
.PHONY: upload_clean compile_clean
upload_clean: SKETCH_NAME = sonar3_clean
upload_clean: upload

compile_clean: SKETCH_NAME = sonar3_clean
compile_clean: compile

# --- Utility targets ---
clean:
	rm -rf $(SKETCH_DIR)

clear: clean

.PHONY: all compile upload clean clear
