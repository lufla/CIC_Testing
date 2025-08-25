import json
import sys
from lib.serial_handler import PortHandler, SerialHandler
from lib import sequence_tester


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

    def run(self):
        """Starts the main execution loop of the QC test suite."""
        # STEP 1: Use PortHandler to find the port
        port_selector = PortHandler()
        self.port = port_selector.get_serial_port()

        # STEP 2: Use the found port to create the SerialHandler
        try:
            print(f"Attempting to connect to {self.port}...")
            self.master_esp = SerialHandler(self.port)
            confirm = input("Is the USB connected to the Master ESP32? (yes/no): ").lower()
            if confirm != 'yes':
                print("Please connect the USB to the Master ESP32 and restart the application.")
                return
            print(f"Successfully connected to Master ESP32 on {self.port}.")
        except Exception as e:
            print(f"Fatal Error: Could not connect to {self.port}. Details: {e}")
            return

        self.serial_number = input("Enter the CIC Serial Number (or scan barcode): ")

        # --- Main application loop ---
        running = True
        while running:
            choice = self._display_menu()
            if choice == '1':
                sequence_tester.run_full_test(self.master_esp, self.serial_number, self.config)

            elif choice in ['2', '3', '4', '5', '6']:
                # For tests 2-6, first get the channel selection
                channel = self._get_channel_choice()
                if channel is None:
                    continue  # If user selected "Back", restart the loop

                if choice == '2':
                    print(f"\n--- Voltage Test for Channel(s): {channel} ---")
                    print(f"Voltage test for channel(s) '{channel}' is not yet implemented.")

                elif choice == '3':
                    print(f"\n--- Current Test for Channel(s): {channel} ---")
                    print(f"Current test for channel(s) '{channel}' is not yet implemented.")

                elif choice == '4':
                    print(f"\n--- Temperature Test for Channel(s): {channel} ---")
                    print(f"Temperature test for channel(s) '{channel}' is not yet implemented.")

                elif choice == '5':
                    print(f"\n--- CAN Test for Channel(s): {channel} ---")
                    print(f"CAN test for channel(s) '{channel}' is not yet implemented.")

                elif choice == '6':
                    print(f"\n--- Crosstalk Test for Channel(s): {channel} ---")
                    print(f"Crosstalk test for channel(s) '{channel}' is not yet implemented.")

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
