# Makefile for Arduino LED blink project (using .ino)
BOARD      = arduino:avr:uno
PORT       = /dev/ttyACM0
SKETCH_DIR = build_sketch
SKETCH_NAME = sonar_main

# The source .ino file
# Allow overriding sketch name: make SKETCH_NAME=sonar2 upload
# Special case for direction_cal_v2: make SKETCH_NAME=direction_cal_v2 upload
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

upload_v2: SKETCH_NAME = direction_cal_v2
upload_v2: upload
	
compile_v2: SKETCH_NAME = direction_cal_v2
compile_v2: compile

upload_v3: SKETCH_NAME = direction_cal_v3
upload_v3: upload
	
compile_v3: SKETCH_NAME = direction_cal_v3
compile_v3: compile

upload_v4: SKETCH_NAME = direction_cal_v4
upload_v4: upload
	
compile_v4: SKETCH_NAME = direction_cal_v4
compile_v4: compile

upload_v5: SKETCH_NAME = direction_cal_v5
upload_v5: upload
	
compile_v5: SKETCH_NAME = direction_cal_v5
compile_v5: compile

upload_final: SKETCH_NAME = direction_final
upload_final: upload
	
compile_final: SKETCH_NAME = direction_final
compile_final: compile

upload_sonar3: SKETCH_NAME = sonar3
upload_sonar3: upload

    compile_sonar3: SKETCH_NAME = sonar3
compile_sonar3: compile

upload_smooth: SKETCH_NAME = direction_smooth
upload_smooth: upload
	
compile_smooth: SKETCH_NAME = direction_smooth
compile_smooth: compile

upload_working: SKETCH_NAME = direction_working
upload_working: upload
	
compile_working: SKETCH_NAME = direction_working
compile_working: compile

upload_clean: SKETCH_NAME = direction_clean
upload_clean: upload
	
compile_clean: SKETCH_NAME = direction_clean
compile_clean: compile

upload_kalman: SKETCH_NAME = direction_kalman
upload_kalman: upload
	
compile_kalman: SKETCH_NAME = direction_kalman
compile_kalman: compile

upload_robust: SKETCH_NAME = direction_robust
upload_robust: upload
	
compile_robust: SKETCH_NAME = direction_robust
compile_robust: compile

upload_fixed: SKETCH_NAME = direction_fixed
upload_fixed: upload
	
compile_fixed: SKETCH_NAME = direction_fixed
compile_fixed: compile

upload_large: SKETCH_NAME = direction_large
upload_large: upload
	
compile_large: SKETCH_NAME = direction_large
compile_large: compile

clean:
	rm -rf $(SKETCH_DIR)

upload_clean_arduino: SKETCH_NAME = clean_upload
upload_clean_arduino: upload

compile_clean_arduino: SKETCH_NAME = clean_upload
compile_clean_arduino: compile

clear: upload_clean_arduino

.PHONY: all compile upload clean clear
