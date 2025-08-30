import time
import math

# DIL switches are in the OFF position.
SWITCHES_OFF_1_25V_CODES = {
    0x03, 0x07, 0x0b, 0x0f, 0x13, 0x17, 0x1b, 0x1f, 0x23, 0x27, 0x2b, 0x2f,
    0x33, 0x37, 0x3b, 0x3f, 0x43, 0x47, 0x4b, 0x4f, 0x53, 0x57, 0x5b, 0x5f,
    0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f, 0x83, 0x87, 0x8b, 0x8f,
    0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf,
    0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef,
    0xf3, 0xf7, 0xfb, 0xff
}

# DIL switches are in the ON position.
# These are the CORRECT, EXPECTED values based on user analysis.
VCAN_1_9V_CODES = {
    0x03, 0x07, 0x0b, 0x13, 0x17, 0x1b, 0x23, 0x27, 0x2b, 0x43, 0x47, 0x4b,
    0x53, 0x57, 0x5b, 0x63, 0x67, 0x6b, 0x83, 0x87, 0x8b, 0x93, 0x97, 0x9b,
    0xa3, 0xa7, 0xab
}
VCAN_2_0V_CODES = {0x0f, 0x1f, 0x2f, 0x4f, 0x5f, 0x6f, 0x8f, 0x9f, 0xaf}
VCAN_2_4V_CODES = {0x33, 0x37, 0x3b, 0x73, 0x77, 0x7b, 0xb3, 0xb7, 0xbb}
VCAN_2_6V_CODES = {0x3f, 0x7f, 0xbf}
VCAN_2_8V_CODES = {0xc3, 0xc7, 0xcb, 0xd3, 0xd7, 0xdb, 0xe3, 0xe7, 0xeb}
VCAN_3_5V_CODES = {0xcf, 0xdf, 0xef}
VCAN_4_2V_CODES = {0xf3, 0xf7, 0xfb}
VCAN_4_6V_CODES = {0xff}


def get_expected_voltage(byte_value, switches_on):
    """
    Calculates the expected voltage based on the corrected, data-driven sets.
    """
    if not switches_on:
        if byte_value in SWITCHES_OFF_1_25V_CODES:
            return 1.25
        else:
            return 0.0
    else:
        # Check against the correct sets for the ON position.
        if byte_value in VCAN_1_9V_CODES: return 1.9
        if byte_value in VCAN_2_0V_CODES: return 2.0
        if byte_value in VCAN_2_4V_CODES: return 2.4
        if byte_value in VCAN_2_6V_CODES: return 2.6
        if byte_value in VCAN_2_8V_CODES: return 2.8
        if byte_value in VCAN_3_5V_CODES: return 3.5
        if byte_value in VCAN_4_2V_CODES: return 4.2
        if byte_value in VCAN_4_6V_CODES: return 4.6

        is_power_on = (byte_value & 0x03) == 0x03
        if is_power_on:
            # If power is on but not in a specific set, it should be 1.25V
            return 1.25

        # Otherwise, power is off.
        return 0.0


def run_test_cycle(ser, switches_on, config):
    """
    Runs through all 256 combinations, accounting for the +1 offset in responses.
    """
    print(f"\n--- Testing all 256 combinations with DIL switches {'ON' if switches_on else 'OFF'} ---")
    time.sleep(0.5)

    # Load all tolerance values from the config file
    tolerance_v = config['settings']['voltage_test_tolerance_v']
    zero_thresh_v = config['settings']['zero_threshold_v']
    high_voltage_tolerance = config['settings']['high_voltage_tolerance_v']  # <-- 1. READ NEW VALUE

    passed_count = 0
    failed_count = 0
    prev_v_a, prev_v_b = -1.0, -1.0

    for byte_val in range(257):
        if byte_val > 0:
            byte_to_check = byte_val - 1
            expected_v = get_expected_voltage(byte_to_check, switches_on)

            # Set the tolerance dynamically based on the expected voltage.
            if math.isclose(expected_v, 0.0):
                current_tolerance = zero_thresh_v
            elif expected_v > 4.0:
                current_tolerance = high_voltage_tolerance
            else:
                current_tolerance = tolerance_v

            fail_a = not math.isclose(prev_v_a, expected_v, abs_tol=current_tolerance)
            fail_b = not math.isclose(prev_v_b, expected_v, abs_tol=current_tolerance)

            if fail_a or fail_b:
                failed_count += 1
                fail_a_str = '(FAIL)' if fail_a else ''
                fail_b_str = '(FAIL)' if fail_b else ''
                print(f"-> FAIL @ {byte_to_check:#04x} (exp: {expected_v:.3f}V, tol: {current_tolerance:.3f}V): "
                      f"Ch A={prev_v_a:.3f}V {fail_a_str}, "
                      f"Ch B={prev_v_b:.3f}V {fail_b_str}")
            else:
                passed_count += 1
                print(
                    f"  OK   @ {byte_to_check:#04x} (exp: {expected_v:.3f}V): Ch A={prev_v_a:.3f}V, Ch B={prev_v_b:.3f}V")

        if byte_val < 256:
            ser.reset_input_buffer()
            command = f"SET_VCAN_VOLTAGE {byte_val}\n"
            ser.write(command.encode('utf-8'))
            ser.flush()
            response = ser.readline().decode('utf-8').strip()

            if not response.startswith("VCAN_DATA:"):
                print(f"Error: Unexpected response for command {byte_val:#04x}: {response}")
                prev_v_a, prev_v_b = -999.0, -999.0
                continue
            try:
                parts = response.split(':')[1].split(',')
                prev_v_a, prev_v_b = map(float, parts)
            except (ValueError, IndexError):
                print(f"Error: Could not parse voltages for command {byte_val:#04x}: {response}")
                prev_v_a, prev_v_b = -999.0, -999.0
                continue

    print(f"\nSummary: Passed={passed_count}/256, Failed={failed_count}/256")
    return failed_count == 0


def run(ser, config):
    """Main function to execute the full voltage channel test."""
    print("\n" + "=" * 40)
    print("         Running Test: Voltage Channels")
    print("=" * 40)

    # --- Part 1: Validation with Switches OFF ---
    input("Ensure all DIL switches are OFF, then press Enter to continue...")
    part1_passed = run_test_cycle(ser, switches_on=False, config=config)
    print(f"\n--- Part 1 (Switches OFF) Result: {'PASS' if part1_passed else 'FAIL'} ---")

    if not part1_passed:
        return False

    # --- Part 2: Validation with Switches ON ---
    input("\nPlease turn ON all DIL switches, then press Enter to continue...")
    part2_passed = run_test_cycle(ser, switches_on=True, config=config)
    print(f"\n--- Part 2 (Switches ON) Result: {'PASS' if part2_passed else 'FAIL'} ---")

    return part2_passed

