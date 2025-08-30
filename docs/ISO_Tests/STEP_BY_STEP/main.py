import serial
import sys
import json
import time
from serial.tools import list_ports

# Assume other test files are in a 'test_functions' sub-directory
from test_functions import initial_checks, voltage_test, current_test, can_test, temperature_test, burnout_test


def load_config():
    """Loads the main configuration file."""
    try:
        with open('config.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print("Error: config.json not found.")
    except json.JSONDecodeError:
        print("Error: Could not parse config.json.")
    return None


def load_check_ranges(config):
    """Loads the explicit min/max ranges for the initial check from the config."""
    try:
        return config['initial_check_ranges']
    except KeyError:
        print("Error: 'initial_check_ranges' section not found in config.json.")
        return None


def select_serial_port():
    """Scans for and allows user to select a serial port."""
    print("Scanning for available serial ports...")
    ports = [p for p in list_ports.comports() if p.device and "n/a" not in p.description.lower()]
    if not ports: return None
    if len(ports) == 1:
        print(f"Auto-connecting to: {ports[0].device}")
        return ports[0].device
    print("Please select a serial port:")
    for i, p in enumerate(ports): print(f"  {i + 1}: {p.device} - {p.description}")
    while True:
        try:
            choice = int(input(f"Enter choice (1-{len(ports)}): ")) - 1
            if 0 <= choice < len(ports): return ports[choice].device
        except (ValueError, IndexError):
            print("Invalid input.")
        except KeyboardInterrupt:
            return None


# ==============================================================================
# FULL TEST SEQUENCE
# ==============================================================================
def run_full_test_sequence(ser, config, ranges):
    """Runs all tests in a predefined sequence and provides a final summary."""
    print("\n--- Starting Full Test Sequence ---")

    test_results = []

    # --- Sequence Start ---

    print("\nStep 1: Performing Setup & Initial Checks...")
    time.sleep(1)
    initial_pass = initial_checks.run(ser, config, ranges)
    test_results.append(("Initial Checks", initial_pass))

    if not initial_pass:
        print("\n--- FULL TEST ABORTED: Initial checks did not pass. ---")
    else:
        # Define the main test suite
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

        for name, test_func, kwargs in test_suite:
            print(f"\n--- Running Test: {name} ---")

            if not initial_checks.run(ser, config, ranges, is_pre_check=True):
                print(f"--- FAILED: Pre-check failed before {name} ---")
                test_results.append((name, False))
                break

            result = test_func(ser, config, **kwargs)
            test_results.append((name, result))

            if not result:
                print(f"--- ABORTING: Critical test failed: {name} ---")
                break

    # --- Final Summary ---
    print("\n" + "=" * 50)
    print("           FULL TEST SEQUENCE SUMMARY")
    print("=" * 50)
    all_passed = True
    for name, result in test_results:
        status_emoji = "✅" if result else "❌"
        print(f"{status_emoji} {name:<40}: {'PASS' if result else 'FAIL'}")
        if not result:
            all_passed = False

    print("-" * 50)
    if all_passed:
        print("✅ Overall Result: ALL TESTS PASSED")
    else:
        print("❌ Overall Result: TEST SEQUENCE FAILED")
        print("   Failed stages:")
        for name, result in test_results:
            if not result:
                print(f"     - {name}")
    print("=" * 50)


# ==============================================================================
# INDIVIDUAL TEST WRAPPERS
# ==============================================================================
def run_voltage_channels_test(ser, config, ranges):
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        result = voltage_test.run(ser, config)
        print(f"\n--- Voltage Channel Test Result: {'PASS' if result else 'FAILED'} ---")
    else:
        print("\n--- Test SKIPPED: Initial checks failed. ---")


def run_current_channels_test(ser, config, ranges):
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        result = current_test.run(ser, config)
        print(f"\n--- Current Channel Test Result: {'PASS' if result else 'FAILED'} ---")
    else:
        print("\n--- Test SKIPPED: Initial checks failed. ---")


def run_temperature_test(ser, config, ranges):
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        temperature_test.run(ser, config)
    else:
        print("\n--- Test SKIPPED: Initial checks failed. ---")


def run_can_channels_test(ser, config, ranges):
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        num_msg = config['can_test_settings']['short_run_messages']
        result = can_test.run(ser, config, num_messages=num_msg)
        print(f"\n--- CAN Communication Test Result: {'PASS' if result else 'FAILED'} ---")
    else:
        print("\n--- Test SKIPPED: Initial checks failed. ---")


def run_burnout_test_wrapper(ser, config, ranges):
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        result = burnout_test.run(ser, config)
        print(f"\n--- Burnout Test Result: {'PASS' if result else 'FAILED'} ---")
    else:
        print("\n--- Test SKIPPED: Initial checks failed. ---")


# ==============================================================================
# MAIN MENU
# ==============================================================================
def main_menu():
    """Displays the main menu and handles user input."""
    config = load_config()
    if not config: sys.exit(1)
    port = select_serial_port()
    if not port: sys.exit(1)
    ranges = load_check_ranges(config)
    if not ranges: sys.exit(1)

    try:
        with serial.Serial(port, config['settings']['baud_rate'], timeout=3) as ser:
            print(f"\nSuccessfully connected to {port}")
            time.sleep(1)
            ser.read_all()
            while True:
                print("\n" + "=" * 40 + "\n           ESP32 Test Suite Menu\n" + "=" * 40)
                print("1. Start Full Test Sequence")
                print("2. Run Setup & Initial Checks")
                print("3. Test Voltage Channels")
                print("4. Test Current Channels")
                print("5. Test Temperature Communication")
                print("6. Test CAN Communication")
                print("7. Run Burnout Test")
                print("8. Exit")
                print("-" * 40)
                choice = input("Enter your choice: ")

                if choice == '1':
                    run_full_test_sequence(ser, config, ranges)
                elif choice == '2':
                    initial_checks.run(ser, config, ranges)
                elif choice == '3':
                    run_voltage_channels_test(ser, config, ranges)
                elif choice == '4':
                    run_current_channels_test(ser, config, ranges)
                elif choice == '5':
                    run_temperature_test(ser, config, ranges)
                elif choice == '6':
                    run_can_channels_test(ser, config, ranges)
                elif choice == '7':
                    run_burnout_test_wrapper(ser, config, ranges)
                elif choice == '8':
                    break
                else:
                    print("Invalid choice, please try again.")

    except serial.SerialException as e:
        print(f"Serial Error: {e}")
    except KeyboardInterrupt:
        print("\nProgram interrupted by user.")
    finally:
        print("Exiting program.")
        sys.exit(0)


if __name__ == "__main__":
    main_menu()

