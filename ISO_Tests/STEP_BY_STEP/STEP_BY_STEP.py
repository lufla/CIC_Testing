import serial
import sys
import json
import time
from serial.tools import list_ports

from test_functions import initial_checks, voltage_test


def run_temperature_comm(ser, config):
    print("\n--- Running Test: Temperature Communication ---")
    print("This test is not yet implemented.")
    time.sleep(1)
    return True


def run_can_comm(ser, config):
    print("\n--- Running Test: CAN Communication ---")
    print("This test is not yet implemented.")
    time.sleep(1)
    return True


def run_crosstalk_comm(ser, config):
    print("\n--- Running Test: Crosstalk Communication ---")
    print("This test is not yet implemented.")
    time.sleep(1)
    return True


def run_current_channels(ser, config):
    print("\n--- Running Test: Current Channels ---")
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


def calculate_ranges(config):
    ranges = {}
    settings = config['settings']
    nominals = config['nominal_values']
    tolerance_factor = settings['tolerance_percent'] / 100.0

    ranges['vcan_v_min'] = settings['powersupply_input_voltage'] * (1 - tolerance_factor)
    ranges['vcan_v_max'] = settings['powersupply_input_voltage'] * (1 + tolerance_factor)
    ranges['cic_v_min'] = nominals['cic_voltage'] * (1 - tolerance_factor)
    ranges['cic_v_max'] = nominals['cic_voltage'] * (1 + tolerance_factor)
    ranges['cic_i_min'] = nominals['cic_current_min_ma'] / 1000.0
    ranges['cic_i_max'] = nominals['cic_current_ma'] / 1000.0 * (1 + tolerance_factor)
    ranges['vcan_i_min'] = nominals['vcan_current_min_ma'] / 1000.0
    ranges['vcan_i_max'] = nominals['vcan_current_ma'] / 1000.0 * (1 + tolerance_factor)

    return ranges


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
        ("Current Channels", run_current_channels),
        ("Temperature Communication", run_temperature_comm),
        ("CAN Communication", run_can_comm),
        ("Crosstalk Communication", run_crosstalk_comm),
    ]

    all_tests_passed = True
    for name, test_func in test_suite:
        # The voltage test has its own internal DIL switch prompts.
        # For other tests, we might need pre-checks, but not for voltage test.
        if name != "Voltage Channels":
            print(f"\n--- Pre-check for {name} ---")
            if not initial_checks.run(ser, config, ranges):
                print(f"--- FAILED: Initial checks did not pass before {name} ---")
                all_tests_passed = False
                break

        # Run the actual test
        result = test_func(ser, config)

        if not result:
            all_tests_passed = False
            print(f"--- FAILED on test: {name} ---")
            break

    print(f"\n--- FULL TEST SEQUENCE COMPLETE ---")
    print(f"Final Result: {'PASS' if all_tests_passed else 'FAILED'}")


def run_voltage_channels_test(ser, config, ranges):
    """Wrapper for the voltage test that includes the initial DIL switch prompt and check."""
    # **FIX:** Added the missing DIL switch prompt here.
    input("\nEnsure all DIL switches are OFF, then press Enter to begin the pre-check...")

    print("\n--- Pre-check for Voltage Test ---")
    if initial_checks.run(ser, config, ranges):
        # If pre-check passes, proceed to the main voltage test
        final_result = voltage_test.run(ser, config)
        print(f"\n--- Voltage Channel Test Result: {'PASS' if final_result else 'FAILED'} ---")
    else:
        print("\n--- Voltage Channel Test SKIPPED: Initial checks failed. ---")


def main_menu():
    config = load_config()
    if not config: sys.exit(1)

    port = select_serial_port()
    if not port: sys.exit(1)

    ranges = calculate_ranges(config)

    try:
        with serial.Serial(port, config['settings']['baud_rate'], timeout=5) as ser:
            print(f"\nSuccessfully connected to {port}")
            time.sleep(1)

            while True:
                print("\n" + "=" * 40 + "\n           ESP32 Test Suite Menu\n" + "=" * 40)
                print("1. Start Full Test Sequence")
                print("2. Run Setup & Initial Checks")
                print("3. Test Voltage Channels")
                print("4. Test Current Channels")
                print("5. Test Temperature Communication")
                print("6. Test CAN Communication")
                print("7. Test Crosstalk Communication")
                print("8. Exit")
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
                    run_current_channels(ser, config)
                elif choice == '5':
                    run_temperature_comm(ser, config)
                elif choice == '6':
                    run_can_comm(ser, config)
                elif choice == '7':
                    run_crosstalk_comm(ser, config)
                elif choice == '8':
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

