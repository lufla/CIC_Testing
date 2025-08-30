import json
import sys
import time
from serial.tools import list_ports

CONFIG_FILE_PATH = 'config.json'

def load_config():
    """Loads the main configuration file."""
    try:
        with open(CONFIG_FILE_PATH, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: Configuration file not found at '{CONFIG_FILE_PATH}'.")
    except json.JSONDecodeError:
        print(f"Error: Could not parse '{CONFIG_FILE_PATH}'. Check for syntax errors.")
    return None

def select_serial_port():
    """Scans for and allows user to select a serial port."""
    print("\nScanning for available serial ports...")
    ports = [p for p in list_ports.comports() if p.device and "n/a" not in p.description.lower()]

    if not ports:
        print("Error: No serial ports found.")
        return None

    if len(ports) == 1:
        print(f"Auto-connecting to: {ports[0].device}")
        return ports[0].device

    print("Please select a serial port:")
    for i, p in enumerate(ports):
        print(f"  {i + 1}: {p.device} - {p.description}")

    while True:
        try:
            choice = input(f"Enter choice (1-{len(ports)}): ")
            if choice.lower() == 'q': return None
            choice_idx = int(choice) - 1
            if 0 <= choice_idx < len(ports):
                return ports[choice_idx].device
            else:
                print("Invalid number.")
        except (ValueError, IndexError):
            print("Invalid input. Please enter a number from the list.")
        except KeyboardInterrupt:
            return None

def read_until_delimiter(ser, delimiter, timeout=1):
    """Reads from serial until a delimiter is found or timeout occurs."""
    start_time = time.time()
    buffer = bytearray()
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            buffer.extend(ser.read(ser.in_waiting))
            if delimiter in buffer:
                line, rest = buffer.split(delimiter, 1)
                return line
    return None