import serial
import sys
import time

from PC_Firmware.lib import session_handler
from lib import utils
from lib.csv_logger import CsvLogger

# Import individual test functions
from test_functions import initial_checks, voltage_test, current_test, can_test, temperature_test, burnout_test


class QCTester:
    """
    The main class that orchestrates the entire Quality Control testing process.
    """

    def __init__(self):
        """Initializes the QCTester, loading configuration and ranges."""
        self.config = utils.load_config()
        if not self.config:
            sys.exit(1)
        self.ranges = self.config.get('initial_check_ranges')
        if not self.ranges:
            print("Error: 'initial_check_ranges' section not found in config.json.")
            sys.exit(1)
        self.ser = None
        self.session_details = {}

    @staticmethod
    def run_full_sequence(ser, config, ranges, session_details, logger):
        # Request and retrieve Master ID and power supply voltage from ESP32
        print("Requesting Master ID from device...")
        ser.write(b"GET_TEST_INFO\n")
        time.sleep(0.5)
        response = utils.read_until_delimiter(ser, b'\n')

        if response and response.startswith(b"TEST_INFO:"):
            parts = response.decode('utf-8').strip().split(':')
            master_id_from_device = parts[1]
            psu_voltage_from_firmware = float(parts[2])
            session_details['master_id'] = master_id_from_device
            session_details['psu_voltage'] = psu_voltage_from_firmware
            print(f"Device Master ID confirmed: {master_id_from_device}")
            print(f"Firmware configured PSU voltage: {psu_voltage_from_firmware}V")
        else:
            print("Failed to retrieve test info from device. Using values from config.")
            # Fallback to the ID from the config file if retrieval fails
            session_details['master_id'] = config['tester_info']['master_id']
            session_details['psu_voltage'] = config['tester_info']['lab_power_supply_voltage_v']

        print("\n" + "=" * 50)
        print("           STARTING FULL TEST SEQUENCE")
        print(
            f"           Operator: {session_details['operator_name']} | S/N: {session_details['serial_number']} | Master ID: {session_details['master_id']}")
        print("=" * 50)

        test_results = []

        # Run initial checks first and foremost.
        initial_pass, initial_data = initial_checks.run(ser, config, ranges, session_details, logger)
        test_results.append(("Initial Checks", initial_pass))
        logger.log_data("Initial Checks", 'PASS' if initial_pass else 'FAIL', session_details, initial_data)

        if not initial_pass:
            print("\n--- FULL TEST ABORTED: Initial checks did not pass. ---")
        else:
            # Define the main test suite to run if initial checks pass
            test_suite = [
                ("Voltage Channels", voltage_test.run, {}),
                ("Current Channels", current_test.run, {}),
                ("Temperature Communication", temperature_test.run, {}),
                ("CAN Communication (Short)", can_test.run,
                 {'num_messages': config['can_test_settings']['short_run_messages']}),
                ("Burnout Test", burnout_test.run, {}),
                ("CAN Communication (Post-Burnout)", can_test.run,
                 {'num_messages': config['can_test_settings']['long_run_messages']})
            ]

            # Execute the test suite
            for name, test_func, kwargs in test_suite:
                print(f"\n--- Running Test: {name} ---")
                # Perform a quick pre-check before each critical test
                pre_check_pass, _ = initial_checks.run(ser, config, ranges, session_details, logger, is_pre_check=True)
                if not pre_check_pass:
                    print(f"--- FAILED: Pre-check failed before {name} ---")
                    test_results.append((name, False))
                    logger.log_data(name, 'FAIL', session_details, {"pre_check_failed": True})
                    break  # Abort the rest of the sequence

                result, test_data = test_func(ser, config, session_details, logger, **kwargs)
                test_results.append((name, result))

                # The log_data call for this test is now handled inside each test function
                # to allow for more granular logging, but we'll still log the overall result here
                # to maintain the overall summary log.

                if not result:
                    print(f"--- ABORTING: Critical test failed: {name} ---")
                    break  # Abort the rest of the sequence

        # --- Final Summary ---
        print("\n" + "=" * 50)
        print("           FULL TEST SEQUENCE SUMMARY")
        print("=" * 50)
        all_passed = all(result for name, result in test_results)
        for name, result in test_results:
            status_emoji = "✅" if result else "❌"
            print(f"{status_emoji} {name:<40}: {'PASS' if result else 'FAIL'}")
        print("-" * 50)

        if all_passed:
            print("✅ Overall Result: ALL TESTS PASSED")
        else:
            print("❌ Overall Result: TEST SEQUENCE FAILED")
            print("   Failed stages:\n     - " + "\n     - ".join([name for name, res in test_results if not res]))
        print("=" * 50)

    def run(self):
        """The main execution loop for the test suite."""
        port = utils.select_serial_port()
        if not port:
            sys.exit(1)

        # Get all operator and device details before connecting
        self.session_details = session_handler.get_and_confirm_details(self.config)
        if not self.session_details:
            print("\nExiting program due to incomplete details.")
            sys.exit(0)

        # Initialize the CSV logger
        logger = CsvLogger()

        try:
            with serial.Serial(port, self.config['settings']['baud_rate'], timeout=3) as ser:
                self.ser = ser
                print(f"\nSuccessfully connected to {port}")
                time.sleep(1)
                self.ser.read_all()
                self._main_menu(logger)

        except serial.SerialException as e:
            print(f"Serial Error: {e}")
        except KeyboardInterrupt:
            print("\nProgram interrupted by user.")
        finally:
            logger.close()  # Ensure the logger file is closed
            print("Exiting program.")
            sys.exit(0)

    def _main_menu(self, logger):
        """Displays the main menu and handles user choices."""
        while True:
            print("\n" + "=" * 40 + "\n           ESP32 Test Suite Menu\n" + "=" * 40)
            print("1. Start Full Test Sequence")
            print("2. Run Setup & Initial Checks")
            print("3. Test Voltage Channels")
            print("4. Test Current Channels")
            print("5. Test Temperature Communication")
            print("6. Test CAN Communication (Short)")
            print("7. Run Burnout Test")
            print("8. Exit")
            print("-" * 40)
            choice = input("Enter your choice: ")

            if choice == '1':
                QCTester.run_full_sequence(self.ser, self.config, self.ranges, self.session_details, logger)
            elif choice == '2':
                initial_checks.run(self.ser, self.config, self.ranges, self.session_details, logger)
            elif choice == '3':
                voltage_test.run(self.ser, self.config, self.session_details, logger)
            elif choice == '4':
                current_test.run(self.ser, self.config, self.session_details, logger)
            elif choice == '5':
                temperature_test.run(self.ser, self.config, self.session_details, logger)
            elif choice == '6':
                can_test.run(self.ser, self.config, self.session_details,
                             logger)  # Runs with default short message count
            elif choice == '7':
                # The burnout test does not have a logger argument as requested.
                burnout_test.run(self.ser, self.config)
            elif choice == '8':
                break
            else:
                print("Invalid choice, please try again.")


if __name__ == "__main__":
    tester = QCTester()
    tester.run()
