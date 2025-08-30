import serial
import sys
import json
import time
from serial.tools import list_ports

from test_functions import initial_checks, voltage_test, current_test, can_test


def run_temperature_comm(ser, config):
    print("\n--- Running Test: Temperature Communication ---")
    print("This test is not yet implemented.")
    time.sleep(1)
    return True


def load_config():
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


def run_full_test_sequence(ser, config, ranges):
    print("\n--- Starting Full Test Sequence ---")
    input("Ensure all DIL switches are OFF, then press Enter...")
    print("\nStep 1: Performing Setup & Initial Checks...")
    if not initial_checks.run(ser, config, ranges):
        print("\n--- FULL TEST FAILED: Initial checks did not pass. ---")
        return

    test_suite = [
        ("Voltage Channels", voltage_test.run),
        ("Current Channels", current_test.run),
        ("Temperature Communication", run_temperature_comm),
        ("CAN & Crosstalk", can_test.run),
    ]
    all_tests_passed = True
    for name, test_func in test_suite:
        print(f"\n--- Pre-check for {name} ---")
        if not initial_checks.run(ser, config, ranges, is_pre_check=True):
            print(f"--- FAILED: Initial checks did not pass before {name} ---")
            all_tests_passed = False
            break
        if name == "Current Channels":
            input("\nEnsure all DIL switches are ON, then press Enter to begin the test...")

        result = test_func(ser, config)
        if not result:
            all_tests_passed = False
            print(f"--- FAILED on test: {name} ---")
            break

    print(f"\n--- FULL TEST SEQUENCE COMPLETE ---")
    print(f"Final Result: {'PASS' if all_tests_passed else 'FAILED'}")


def run_voltage_channels_test(ser, config, ranges):
    """Wrapper for the voltage test with its pre-check."""
    print("\n--- Pre-check for Voltage Test ---")
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        final_result = voltage_test.run(ser, config)
        print(f"\n--- Voltage Channel Test Result: {'PASS' if final_result else 'FAILED'} ---")
    else:
        print("\n--- Voltage Channel Test SKIPPED: Initial checks failed. ---")


def run_current_channels_test(ser, config, ranges):
    """Wrapper for the current test with its pre-check."""
    input("\nEnsure all DIL switches are ON, then press Enter to begin the pre-check...")
    print("\n--- Pre-check for Current Test ---")
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        final_result = current_test.run(ser, config)
        print(f"\n--- Current Channel Test Result: {'PASS' if final_result else 'FAILED'} ---")
    else:
        print("\n--- Current Channel Test SKIPPED: Initial checks failed. ---")


def run_can_channels_test(ser, config, ranges):
    """Wrapper for the CAN test with its pre-check."""
    print("\n--- Pre-check for CAN Test ---")
    if initial_checks.run(ser, config, ranges, is_pre_check=True):
        final_result = can_test.run(ser, config)
        print(f"\n--- CAN & Crosstalk Test Result: {'PASS' if final_result else 'FAILED'} ---")
    else:
        print("\n--- CAN Test SKIPPED: Initial checks failed. ---")


def main_menu():
    config = load_config()
    if not config: sys.exit(1)
    port = select_serial_port()
    if not port: sys.exit(1)

    ranges = load_check_ranges(config)
    if not ranges: sys.exit(1)

    try:
        with serial.Serial(port, config['settings']['baud_rate'], timeout=2) as ser:
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
                print("6. Test CAN & Crosstalk")
                print("7. Exit")
                print("-" * 40)
                choice = input("Enter your choice: ")
                test_was_run = True
                if choice == '1':
                    run_full_test_sequence(ser, config, ranges)
                elif choice == '2':
                    input("Ensure all DIL switches are OFF, then press Enter...")
                    initial_checks.run(ser, config, ranges)
                elif choice == '3':
                    run_voltage_channels_test(ser, config, ranges)
                elif choice == '4':
                    run_current_channels_test(ser, config, ranges)
                elif choice == '5':
                    run_temperature_comm(ser, config)
                elif choice == '6':
                    run_can_channels_test(ser, config, ranges)
                elif choice == '7':
                    break
                else:
                    test_was_run = False
                    print("Invalid choice, please try again.")
                if test_was_run:
                    print("\nReturning to menu in 3 seconds...")
                    time.sleep(3)
    except serial.SerialException as e:
        print(f"Serial Error: {e}")
    except KeyboardInterrupt:
        print("\nProgram interrupted by user.")
    finally:
        print("Exiting program.")
        sys.exit(0)


if __name__ == "__main__":
    main_menu()

