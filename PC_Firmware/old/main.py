import json
import sys
import re
from datetime import datetime
from lib.serial_handler import PortHandler, SerialHandler
from lib import sequence_tester
from lib import individual_tests  # Import the new test functions


class QCTester:
    """
    A class to encapsulate the entire Quality Control testing process.
    """
    CONFIG_FILE_PATH = 'config.jason'

    def __init__(self):
        """Initializes the QCTester."""
        print("--- Initializing Test Environment ---")
        self.config = self._load_config()
        self.port = None
        self.master_esp = None
        self.serial_number = None
        # This dictionary will hold the confirmed details for the entire test session
        self.session_details = {}

    def _load_config(self):
        """Loads the configuration from the specified JSON file."""
        try:
            with open(self.CONFIG_FILE_PATH, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"Error: Configuration file not found at '{self.CONFIG_FILE_PATH}'.")
            sys.exit(1)
        except json.JSONDecodeError:
            print(f"Error: Could not decode JSON from '{self.CONFIG_FILE_PATH}'. Please check for syntax errors.")
            sys.exit(1)

    def _display_menu(self):
        """Displays the main menu and returns the user's choice."""
        print("\n--- CIC Quality Control Test ---")
        print("1. Start Full Test Sequence")
        print("2. Test Voltage Channels")
        print("3. Test Current Channels")
        print("4. Test Temperature Communication")
        print("5. Test CAN Communication")
        print("6. Test Crosstalk Communication")
        print("7. Exit")
        while True:
            choice = input("Enter your choice (1-7): ")
            if choice in ['1', '2', '3', '4', '5', '6', '7']:
                return choice
            else:
                print("Invalid choice.")

    def _get_channel_choice(self):
        """
        Displays a sub-menu to select a channel and returns the user's choice.
        Returns 'A', 'B', 'Both', or None if the user wants to go back.
        """
        print("\nWhich channel do you want to test?")
        print("  1. Channel A")
        print("  2. Channel B")
        print("  3. Both")
        print("  4. Back to Main Menu")
        while True:
            choice = input("Enter your choice (1-4): ")
            if choice == '1':
                return 'A'
            elif choice == '2':
                return 'B'
            elif choice == '3':
                return 'Both'
            elif choice == '4':
                return None  # Use None as a signal to return to the main menu
            else:
                print("Invalid choice. Please enter a number between 1 and 4.")

    def _validate_input(self, value, pattern, error_message):
        """A generic validation helper using regex."""
        if re.match(pattern, value):
            return True
        print(error_message)
        return False

    def _get_and_confirm_details(self):
        """
        Gets the serial number, displays all test details, and asks for user confirmation.
        Allows for manual, validated changes to the details.
        """
        # Get validation settings from the config file
        test_params = self.config.get('test_parameters', {})
        validation_rules = self.config.get('tester_info', {}).get('validation_rules', {})

        sn_len = test_params.get('serial_number_length', 4)
        sn_numeric = test_params.get('serial_number_numeric_only', False)

        id_prefix = validation_rules.get('station_id_prefix', 'QC-Station-')
        id_num_len = validation_rules.get('station_id_numeric_length', 2)

        # --- Initial Serial Number Entry ---
        while True:
            self.serial_number = input(f"Enter the CIC Serial Number ({sn_len} characters): ")
            if len(self.serial_number) != sn_len:
                print(f"Error: Serial number must be {sn_len} characters long.")
                continue
            if sn_numeric and not self.serial_number.isdigit():
                print("Error: Serial number must only contain numbers.")
                continue
            break

        # --- Prepare and Confirm Details ---
        current_time = datetime.now().strftime('%H:%M:%S')
        current_date = datetime.now().strftime('%Y-%m-%d')
        operator_name = self.config['tester_info']['operator_name']
        master_id = self.master_esp.station_id

        while True:
            print("\n--- Please Confirm Test Details ---")
            print(f"1. Date:          {current_date}")
            print(f"2. Time:          {current_time}")
            print(f"3. Operator Name: {operator_name}")
            print(f"4. Master ID:     {master_id}")
            print(f"5. Serial Number: {self.serial_number}")
            print("------------------------------------")

            confirm = input("Are these details correct? (yes/no): ").lower()
            if confirm == 'yes':
                self.session_details = {
                    'date': current_date, 'time': current_time, 'operator_name': operator_name,
                    'master_id': master_id, 'serial_number': self.serial_number
                }
                print("Details confirmed.")
                return True
            elif confirm == 'no':
                while True:
                    choice = input("Which value to change? (1-5, or 'done' to finish): ").lower()
                    if choice == '1':  # Date
                        while True:
                            new_date = input("Enter new Date (YYYY-MM-DD): ")
                            if self._validate_input(new_date, r'^\d{4}-\d{2}-\d{2}$',
                                                    "Error: Format must be YYYY-MM-DD."):
                                current_date = new_date
                                break
                    elif choice == '2':  # Time
                        while True:
                            new_time = input("Enter new Time (HH:MM:SS): ")
                            if self._validate_input(new_time, r'^\d{2}:\d{2}:\d{2}$',
                                                    "Error: Format must be HH:MM:SS."):
                                current_time = new_time
                                break
                    elif choice == '3':  # Operator Name
                        while True:
                            new_name = input("Enter new Operator Name (letters and spaces only): ")
                            if new_name.strip() and self._validate_input(new_name, r'^[A-Za-z\s]+$',
                                                                         "Error: Name must only contain letters and spaces."):
                                operator_name = new_name
                                break
                    elif choice == '4':  # Master ID
                        while True:
                            new_id = input(f"Enter new Master ID (e.g., {id_prefix}01): ")
                            if not new_id.startswith(id_prefix):
                                print(f"Error: ID must start with '{id_prefix}'.")
                                continue
                            numeric_part = new_id[len(id_prefix):]
                            if not (numeric_part.isdigit() and len(numeric_part) == id_num_len):
                                print(f"Error: The part after the prefix must be {id_num_len} digits.")
                                continue
                            master_id = new_id
                            break
                    elif choice == '5':  # Serial Number
                        while True:
                            new_sn = input(f"Enter new Serial Number ({sn_len} characters): ")
                            if len(new_sn) != sn_len:
                                print(f"Error: Serial number must be {sn_len} characters long.")
                                continue
                            if sn_numeric and not new_sn.isdigit():
                                print("Error: Serial number must only contain numbers.")
                                continue
                            self.serial_number = new_sn
                            break
                    elif choice == 'done':
                        break
                    else:
                        print("Invalid choice. Please enter a number between 1 and 5, or 'done'.")
            else:
                print("Invalid input. Please enter 'yes' or 'no'.")

    def run(self):
        """Starts the main execution loop of the QC test suite."""
        port_selector = PortHandler()
        self.port = port_selector.get_serial_port()

        try:
            baudrate = self.config['test_parameters']['serial_baudrate']
            print(f"Attempting to connect to {self.port} at {baudrate} baud...")
            self.master_esp = SerialHandler(self.port, baudrate=baudrate)

            if not self.master_esp.request_device_info():
                print("Fatal Error: Could not retrieve Station ID from the device.")
                self._cleanup()
                return

            confirm = input("Is the USB connected to the Master ESP32? (yes/no): ").lower()
            if confirm != 'yes':
                print("Please connect the USB to the Master ESP32 and restart the application.")
                self._cleanup()
                return
            print(f"Successfully connected to Master ESP32 on {self.port}.")
        except Exception as e:
            print(f"Fatal Error: Could not connect to {self.port}. Details: {e}")
            self._cleanup()
            return

        if not self._get_and_confirm_details():
            self._cleanup()
            return

        running = True
        while running:
            choice = self._display_menu()
            if choice == '1':
                sequence_tester.run_full_test(self.master_esp, self.session_details, self.config)

            elif choice == '2':  # Voltage Test
                channel = self._get_channel_choice()
                if channel:  # A, B, or Both
                    individual_tests.run_voltage_test(self.master_esp, self.session_details, self.config, channel)

            elif choice == '3':  # Current Test
                channel = self._get_channel_choice()
                if channel:
                    individual_tests.run_current_test(self.master_esp, self.session_details, self.config, channel)

            elif choice == '4':  # Temperature Test
                individual_tests.run_temperature_test(self.master_esp, self.session_details, self.config)

            elif choice == '5':  # CAN Test
                channel = self._get_channel_choice()
                if channel:
                    individual_tests.run_can_test(self.master_esp, self.session_details, self.config, channel)

            elif choice == '6':  # Crosstalk Test
                channel = self._get_channel_choice()
                if channel:
                    individual_tests.run_crosstalk_test(self.master_esp, self.session_details, self.config, channel)

            elif choice == '7':
                running = False
        self._cleanup()

    def _cleanup(self):
        """Closes the serial connection."""
        if self.master_esp:
            self.master_esp.close()
        print("\nTesting complete. Connection closed.")


def main():
    """Main function to create and run the QC tester instance."""
    tester = QCTester()
    tester.run()


if __name__ == "__main__":
    main()
