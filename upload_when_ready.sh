#!/bin/bash

echo "Waiting for Arduino serial port to become available..."

# Wait for port to be available
while [ ! -c "/dev/ttyACM0" ]; do
    sleep 1
    echo "."
done

# Give it a moment to stabilize
sleep 2

echo "Port available! Uploading sonar3..."
make upload_sonar3

echo "Upload process completed."