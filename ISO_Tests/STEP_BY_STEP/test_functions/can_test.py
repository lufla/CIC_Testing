import serial
import time


def _calculate_voltage_code(target_voltage):
    """
    Calculates the integer code to send to the ESP32 for a target voltage.
    NOTE: This is based on an assumed linear 8-bit DAC with a 3.3V reference.
    The two least significant bits (0x03) are flags to enable power.
    You may need to adjust the V_MAX value if your hardware is different.
    """
    V_MAX = 3.3
    if not 0 < target_voltage <= V_MAX:
        return 0  # Return 0 (power off) for invalid targets

    # Scale voltage to the 6 bits available for voltage level (bits 2-7)
    voltage_bits = int((target_voltage / V_MAX) * 63)

    # Shift left by 2 and add the power-on flag
    code = (voltage_bits << 2) | 0x03
    return code


def _set_and_verify_vcan(ser, config):
    """Sets VCAN to the target voltage and verifies it's stable."""
    target_v = config['can_test_settings']['vcan_target_voltage']
    settle_time = config['can_test_settings']['voltage_settle_time_s']
    tolerance = config['settings']['voltage_test_tolerance_v']

    code = _calculate_voltage_code(target_v)
    if code == 0:
        print(f"Error: Invalid target voltage {target_v}V specified in config.")
        return False

    print(f"Setting VCAN to {target_v}V (Code: {code})...")
    ser.write(f"SET_VCAN_VOLTAGE {code}\n".encode())

    time.sleep(settle_time)  # Wait for voltage to stabilize

    # Read back and verify
    line = ser.readline().decode('utf-8').strip()
    if not line.startswith("VCAN_DATA:"):
        print(f"Error: Did not receive VCAN_DATA confirmation. Got: {line}")
        return False

    try:
        _, values = line.split(':')
        v_a, v_b = map(float, values.split(','))
        print(f"  > Read back: VCAN_A={v_a:.4f}V, VCAN_B={v_b:.4f}V")
        if abs(v_a - target_v) > tolerance or abs(v_b - target_v) > tolerance:
            print("Error: VCAN voltage did not settle within tolerance.")
            return False
    except (ValueError, IndexError):
        print(f"Error: Could not parse VCAN data: {line}")
        return False

    print("VCAN voltage is stable. Starting test.")
    return True


def perform_can_test(ser, num_messages):
    """
    Sends the CAN test command to the ESP32 and waits for a final result.
    """
    print(f"Sending command to ESP32 to test with {num_messages} messages...")
    ser.write(f"RUN_CAN_TEST {num_messages}\n".encode())

    timeout = time.time() + 2.0 + (num_messages * 0.05)
    while time.time() < timeout:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line:
            continue
        if "CAN_TEST_PROGRESS" in line:
            print(f"  > {line}")
        elif line.startswith("CAN_TEST_FINAL:"):
            if "PASS" in line:
                print(f"  > {line}")
                return True
            else:
                print(f"Error: Test failed. Reason from device: {line}")
                return False
    print("\nError: Serial timeout. Did not receive a final result from the ESP32.")
    return False


def run(ser, config):
    """
    Main function to run the complete CAN & Crosstalk test suite.
    """
    print("\n--- Running Test: CAN & Crosstalk Communication ---")

    try:
        # --- Phase 1: Power up VCAN ---
        if not _set_and_verify_vcan(ser, config):
            return False  # Stop if voltage can't be set

        # --- Phase 2: Short Test ---
        short_run_count = config['can_test_settings']['short_run_messages']
        print(f"\nPhase 1: Running with {short_run_count} messages...")
        if not perform_can_test(ser, short_run_count):
            print(f"--- FAILED: CAN test ({short_run_count} messages) did not pass. ---")
            return False
        print("--- Phase 1 PASSED ---")
        time.sleep(1)

        # --- Phase 3: Long Test ---
        long_run_count = config['can_test_settings']['long_run_messages']
        print(f"\nPhase 2: Running with {long_run_count} messages...")
        if not perform_can_test(ser, long_run_count):
            print(f"--- FAILED: CAN test ({long_run_count} messages) did not pass. ---")
            return False
        print("--- Phase 2 PASSED ---")

        print("\n--- CAN & Crosstalk Test Fully PASSED ---")
        return True

    finally:
        # --- Cleanup: Always turn off VCAN ---
        print("\nCleaning up: Turning off VCAN power...")
        ser.write(b"SET_VCAN_VOLTAGE 0\n")
        time.sleep(0.5)
        # Read the confirmation but don't strictly check it during cleanup
        ser.read_all()

