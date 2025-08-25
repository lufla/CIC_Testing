import sys
import time
import json
import serial
import serial.tools.list_ports


class PortHandler:
    """
    Handles the detection and selection of serial ports.
    Its single responsibility is to find and return a port name.
    """

    def __init__(self):
        """Initializes the PortHandler."""
        self.ports = []

    def _find_ports(self):
        """Scans for and stores a list of available serial ports."""
        print("Detecting available serial ports...")
        self.ports = [p for p in serial.tools.list_ports.comports() if "n/a" not in p.description.lower()]

    def _select_port_from_list(self):
        """
        Prompts the user to select a port from a list if multiple are found.
        Returns the device path of the selected port.
        """
        print("\nMultiple serial ports detected. Please choose one:")
        for i, port_info in enumerate(self.ports):
            # Provides detailed info for easier identification
            print(f"  {i + 1}: {port_info.device} - {port_info.description}")

        while True:
            try:
                choice = int(input(f"Enter your choice (1-{len(self.ports)}): "))
                if 1 <= choice <= len(self.ports):
                    return self.ports[choice - 1].device
                else:
                    print("Invalid choice. Please select a number from the list.")
            except ValueError:
                print("Invalid input. Please enter a number.")

    def get_serial_port(self):
        """
        Main method to get the target serial port.
        - Automatically selects if only one is found.
        - Prompts for selection if multiple are found.
        - Exits if no ports are found.
        """
        self._find_ports()

        if not self.ports:
            print("Error: No serial ports detected. Please ensure your device is connected.")
            sys.exit(1)

        if len(self.ports) == 1:
            port = self.ports[0].device
            print(f"Automatically selected the only available port: {port}")
            return port

        return self._select_port_from_list()


class SerialHandler:
    """
    Handles all communication over a given serial port.
    Its single responsibility is to send and receive data.
    """

    def __init__(self, port, baudrate=115200, timeout=2):
        """
        Initializes the serial connection to the ESP32.

        Args:
            port (str): The COM port or device path (e.g., 'COM3' or '/dev/ttyUSB0').
            baudrate (int): The baud rate for the connection.
            timeout (int): The read timeout in seconds.
        """
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        self.station_id = None
        print("Opening serial port. Waiting for device.")
        time.sleep(2)  # Wait for the ESP32 to reset after the serial connection is established.
        self.ser.flushInput()  # Flush any startup messages from the ESP32 buffer

    def request_device_info(self):
        """
        Asks the device for its identification info (like station_id).

        Returns:
            bool: True if the info was received successfully, False otherwise.
        """
        print("Requesting device info...")
        response = self.send_command({"command": "get_info"})
        if response and response.get('status') == 'info':
            self.station_id = response.get('station_id', 'Unknown ID')
            print(f"--> Received info. Station ID: {self.station_id}")
            return True
        else:
            print(f"--> Failed to get device info. Response: {response}")
            return False

    def send_command(self, command_dict):
        """
        Sends a command dictionary to the ESP32 as a JSON string.

        Args:
            command_dict (dict): The command to send.

        Returns:
            dict: The JSON response from the ESP32, or an error dictionary.
        """
        if self.ser and self.ser.is_open:
            try:
                # Always flush the input buffer before writing to ensure we get the correct response
                self.ser.flushInput()
                command_json = json.dumps(command_dict)
                print(f"--> [SENDING] {command_json}") # Print the command being sent
                self.ser.write((command_json + '\n').encode('utf-8'))
                return self.read_response()
            except Exception as e:
                return {"status": "error", "message": f"Failed to send command: {e}"}
        else:
            return {"status": "error", "message": "Serial port not open."}

    def read_response(self):
        """
        Waits for and reads a complete JSON response from the ESP32.

        Returns:
            dict: The parsed JSON response, or an error dictionary.
        """
        response_line = ""
        try:
            response_line = self.ser.readline().decode('utf-8').strip()
            print(f"<-- [RECEIVED] {response_line}") # Print the raw response
            if response_line:
                return json.loads(response_line)
            # This happens if the timeout is reached
            return {"status": "error", "message": "Timeout: No response from device."}
        except json.JSONDecodeError:
            # The raw response is already printed, so the error message is more informative
            return {"status": "error", "message": "Invalid JSON in response."}
        except Exception as e:
            return {"status": "error", "message": f"Error reading response: {e}"}

    def close(self):
        """Closes the serial port."""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Serial port closed.")
