import serial
import time
import sys
import json
from serial.tools import list_ports


def load_config():
    """Reads settings from config.json and returns the parsed data."""
    try:
        with open('config.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print("Error: config.json not found. Please create it in the same directory.")
    except json.JSONDecodeError:
        print("Error: Could not parse config.json. Please check its format for errors.")
    except Exception as e:
        print(f"An unexpected error occurred while reading config.json: {e}")
    return None


def calculate_ranges(config):
    """Calculates min/max acceptable values based on the configuration."""
    ranges = {}
    settings = config['settings']
    nominals = config['nominal_values']

    tolerance_factor = settings['tolerance_percent'] / 100.0

    # VCAN Voltage
    nominal_vcan_v = settings['powersupply_input_voltage']
    ranges['vcan_v_min'] = nominal_vcan_v * (1 - tolerance_factor)
    ranges['vcan_v_max'] = nominal_vcan_v * (1 + tolerance_factor)

    # CIC Voltage
    nominal_cic_v = nominals['cic_voltage']
    ranges['cic_v_min'] = nominal_cic_v * (1 - tolerance_factor)
    ranges['cic_v_max'] = nominal_cic_v * (1 + tolerance_factor)

    # CIC Current (convert mA to A for checking)
    nominal_cic_i = nominals['cic_current_ma'] / 1000.0
    ranges['cic_i_min'] = nominals['cic_current_min_ma'] / 1000.0
    ranges['cic_i_max'] = nominal_cic_i * (1 + tolerance_factor)

    # VCAN Current (convert mA to A for checking)
    nominal_vcan_i = nominals['vcan_current_ma'] / 1000.0
    ranges['vcan_i_min'] = nominals['vcan_current_min_ma'] / 1000.0
    ranges['vcan_i_max'] = nominal_vcan_i * (1 + tolerance_factor)

    return ranges


def select_serial_port():
    """Scans for and selects a serial port."""
    print("Scanning for available serial ports...")
    ports = list_ports.comports()
    available_ports = [p for p in ports if p.device and (not p.description or "n/a" not in p.description.lower())]

    if not available_ports:
        print("Error: No usable serial ports found.")
        return None
    if len(available_ports) == 1:
        print(f"Auto-connecting to: {available_ports[0].device}")
        return available_ports[0].device

    print("Multiple serial ports found. Please select one:")
    for i, p in enumerate(available_ports):
        print(f"  {i + 1}: {p.device} - {p.description}")

    while True:
        try:
            choice = int(input(f"Enter your choice (1-{len(available_ports)}): ")) - 1
            if 0 <= choice < len(available_ports):
                return available_ports[choice].device
        except (ValueError, IndexError):
            print("Invalid input.")
        except KeyboardInterrupt:
            return None


def check_value(name, value, v_min, v_max):
    """Checks if a single value is within its range. Returns True if failed."""
    if not (v_min <= value <= v_max):
        print(f"-> FAILED! {name}: {value:.4f} is outside the expected range ({v_min:.4f} - {v_max:.4f})")
        return True
    return False


def setup_and_initial_checks(serial_port, config, ranges):
    """Guides user and runs the comprehensive ADC check."""
    try:
        baud_rate = config['settings']['baud_rate']
        duration = config['settings']['initial_voltage_duration']

        with serial.Serial(serial_port, baud_rate, timeout=2) as ser:
            print(f"Successfully connected to {serial_port}")
            time.sleep(2)

            print("\n--- Setup & Initial Checks ---")
            input("Ensure all DIL switches are OFF, then press Enter...")

            print("\n--- Expected Ranges ---")
            print(f"  CIC Voltage:  {ranges['cic_v_min']:.3f}V - {ranges['cic_v_max']:.3f}V")
            print(f"  CIC Current:  {ranges['cic_i_min'] * 1000:.1f}mA - {ranges['cic_i_max'] * 1000:.1f}mA")
            print(f"  VCAN Voltage: {ranges['vcan_v_min']:.3f}V - {ranges['vcan_v_max']:.3f}V")
            print(f"  VCAN Current: {ranges['vcan_i_min'] * 1000:.1f}mA - {ranges['vcan_i_max'] * 1000:.1f}mA")

            print(f"\nStarting check for {duration} seconds...")
            time.sleep(3)
            print("--- CHECKING NOW ---")

            ser.write(b'CHECK_SPI_ADC\n')
            start_time = time.time()
            test_passed = True
            data_received = False

            while time.time() - start_time < duration:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').strip()
                    if line.startswith("DATA:"):
                        data_received = True
                        try:
                            parts = line.split(':')[1].split(',')
                            cic_v, cic_i, vcan_v, vcan_i = map(float, parts)

                            print(
                                f"  OK: CIC V: {cic_v:.3f}V, I: {cic_i * 1000:.1f}mA | VCAN V: {vcan_v:.3f}V, I: {vcan_i * 1000:.1f}mA")

                            failure = False
                            if check_value("CIC Voltage", cic_v, ranges['cic_v_min'],
                                           ranges['cic_v_max']): failure = True
                            if check_value("CIC Current", cic_i, ranges['cic_i_min'],
                                           ranges['cic_i_max']): failure = True
                            if check_value("VCAN Voltage", vcan_v, ranges['vcan_v_min'],
                                           ranges['vcan_v_max']): failure = True
                            if check_value("VCAN Current", vcan_i, ranges['vcan_i_min'],
                                           ranges['vcan_i_max']): failure = True

                            if failure:
                                test_passed = False
                                break

                        except (ValueError, IndexError):
                            print(f"Warning: Could not parse data: {line}")

                time.sleep(0.1)

            print("--- TEST COMPLETE ---")
            if not data_received:
                print("Result: FAILED - No data received from slave. Check connections.")
            elif test_passed:
                print("Result: PASS")
            else:
                print("Result: FAILED")

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nTest interrupted.")


if __name__ == "__main__":
    app_config = load_config()
    if app_config:
        value_ranges = calculate_ranges(app_config)
        port = select_serial_port()
        if port:
            setup_and_initial_checks(port, app_config, value_ranges)

    print("Exiting program.")
    sys.exit(1)

