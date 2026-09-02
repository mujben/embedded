import serial
import time

SERIAL_PORT = "COM3" # ESP32 serial port
BAUD_RATE = 115200
TOTAL_BYTES = 8 * 1024 * 1024 # 8 MB
OUTPUT_FILE = "firmware_dump.bin"

print(f"Opening port {SERIAL_PORT}")
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5)

print("Waiting for ESP32 to send data")

with open(OUTPUT_FILE, "wb") as f:
    received = 0
    while received < TOTAL_BYTES:

        chunk = ser.read(min(512, TOTAL_BYTES - received))
        if not chunk:
            print("\nTimeout. No further data received.")
            break
        f.write(chunk)
        received += len(chunk)

print(f"\nRead {received} bytes to file {OUTPUT_FILE}")
ser.close()