import time
import math

#DIL switches are in the OFF position.
# This set contains the hex codes that were empirically discovered to produce
# ~1.25V when the DIL switches are in the OFF position. This is the key to
# creating an accurate test that matches the real hardware behavior.
# all other hex values from 0x00 to 0xFF have the Voltage 0
SWITCHES_OFF_1_25V_CODES = {
    0x68, 0x6c, 0x72, 0x76, 0x7c, 0x80, 0x86, 0x8a, 0x90, 0x94, 0x9a, 0x9e,
    0xa4, 0xa8, 0xae, 0xb2, 0xb8, 0xbc, 0xc2, 0xc6, 0xcc, 0xd0, 0xd6, 0xda,
    0xe0, 0xe4, 0xea, 0xee, 0xf4, 0xf8, 0xfe
}

# DIL switches are in the ON position.
# This set contains the hex codes that produce ~0V. This occurs when
# the power enable bits (Bit 0 and Bit 1) are not both set to 1.
# This corresponds to any hex value not ending in 3, 7, B, or F.
VCAN_0_0V_CODES = {
    0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0c, 0x0d, 0x0e,
    0x10, 0x11, 0x12, 0x14, 0x15, 0x16, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x1e,
    0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2c, 0x2d, 0x2e,
    0x30, 0x31, 0x32, 0x34, 0x35, 0x36, 0x38, 0x39, 0x3a, 0x3c, 0x3d, 0x3e,
    0x40, 0x41, 0x42, 0x44, 0x45, 0x46, 0x48, 0x49, 0x4a, 0x4c, 0x4d, 0x4e,
    0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5a, 0x5c, 0x5d, 0x5e,
    0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0x68, 0x69, 0x6a, 0x6c, 0x6d, 0x6e,
    0x70, 0x71, 0x72, 0x74, 0x75, 0x76, 0x78, 0x79, 0x7a, 0x7c, 0x7d, 0x7e,
    0x80, 0x81, 0x82, 0x84, 0x85, 0x86, 0x88, 0x89, 0x8a, 0x8c, 0x8d, 0x8e,
    0x90, 0x91, 0x92, 0x94, 0x95, 0x96, 0x98, 0x99, 0x9a, 0x9c, 0x9d, 0x9e,
    0xa0, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6, 0xa8, 0xa9, 0xaa, 0xac, 0xad, 0xae,
    0xb0, 0xb1, 0xb2, 0xb4, 0xb5, 0xb6, 0xb8, 0xb9, 0xba, 0xbc, 0xbd, 0xbe,
    0xc0, 0xc1, 0xc2, 0xc4, 0xc5, 0xc6, 0xc8, 0xc9, 0xca, 0xcc, 0xcd, 0xce,
    0xd0, 0xd1, 0xd2, 0xd4, 0xd5, 0xd6, 0xd8, 0xd9, 0xda, 0xdc, 0xdd, 0xde,
    0xe0, 0xe1, 0xe2, 0xe4, 0xe5, 0xe6, 0xe8, 0xe9, 0xea, 0xec, 0xed, 0xee,
    0xf0, 0xf1, 0xf2, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfc, 0xfd, 0xfe
}

# This set contains the hex codes that were measured to produce ~1.9V.
VCAN_1_9V_CODES = {
    0x03, 0x07, 0x0b, 0x13, 0x17, 0x1b, 0x23, 0x27, 0x2b, 0x43, 0x47, 0x4b,
    0x53, 0x57, 0x5b, 0x63, 0x67, 0x6b, 0x83, 0x87, 0x8b, 0x93, 0x97, 0x9b,
    0xa3, 0xa7, 0xab
}

# This set contains the hex codes that were measured to produce ~2.0V.
VCAN_2_0V_CODES = {
    0x0f, 0x1f, 0x2f, 0x4f, 0x5f, 0x6f, 0x8f, 0x9f, 0xaf
}

# This set contains the hex codes that were measured to produce ~2.4V.
VCAN_2_4V_CODES = {
    0x33, 0x37, 0x3b, 0x73, 0x77, 0x7b, 0xb3, 0xb7, 0xbb
}

# This set contains the hex codes that were measured to produce ~2.6V.
VCAN_2_6V_CODES = {
    0x3f, 0x7f, 0xbf
}

# This set contains the hex codes that were measured to produce ~2.8V.
VCAN_2_8V_CODES = {
    0xc3, 0xc7, 0xcb, 0xd3, 0xd7, 0xdb, 0xe3, 0xe7, 0xeb
}

# This set contains the hex codes that were measured to produce ~3.5V.
VCAN_3_5V_CODES = {
    0xcf, 0xdf, 0xef
}

# This set contains the hex codes that were measured to produce ~4.2V.
VCAN_4_2V_CODES = {
    0xf3, 0xf7, 0xfb
}

# This set contains the hex code that was measured to produce ~4.6V.
VCAN_4_6V_CODES = {
    0xff
}


def get_expected_voltage(byte_value, switches_on):
    """
    Calculates the expected voltage based on the 8-bit setting.
    This logic is now a hybrid of the PDF and the empirically discovered data.
    """
    if not switches_on:
        # When switches are OFF, the logic is based on the discovered hex codes.
        if byte_value in SWITCHES_OFF_1_25V_CODES:
            return 1.25
        else:
            return 0.0

    # --- Logic for when switches are ON (based on the PDF) ---
    power_enabled = (byte_value & 0x03) == 0x03
    if not power_enabled:
        return 0.0

    a_set = (byte_value & 0x0C) == 0x0C
    b_set = (byte_value & 0x30) == 0x30
    c_set = (byte_value & 0xC0) == 0xC0

    if c_set and b_set and a_set: return 4.6
    if c_set and b_set and not a_set: return 4.2
    if c_set and not b_set and a_set: return 3.5
    if c_set and not b_set and not a_set: return 2.8
    if not c_set and b_set and a_set: return 2.6
    if not c_set and b_set and not a_set: return 2.4
    if not c_set and not b_set and a_set: return 2.0
    if not c_set and not b_set and not a_set: return 1.9

    # Fallback if no specific resistor combo is set, but power is on.
    return 1.25


def run_test_cycle(ser, switches_on, config):
    """Runs through all 256 combinations using absolute voltage tolerance."""
    print(f"\n--- Testing all 256 combinations with DIL switches {'ON' if switches_on else 'OFF'} ---")

    tolerance_v = config['settings']['voltage_test_tolerance_v']
    zero_thresh_v = config['settings']['zero_threshold_v']

    passed_count = 0
    failed_count = 0

    for byte_val in range(256):
        ser.reset_input_buffer()
        command = f"SET_VCAN_VOLTAGE {byte_val}\n"
        ser.write(command.encode('utf-8'))

        response = ser.readline().decode('utf-8').strip()
        if not response.startswith("VCAN_DATA:"):
            print(f"Error: Unexpected response for byte {byte_val:#04x}: {response}")
            failed_count += 1
            continue

        try:
            parts = response.split(':')[1].split(',')
            v_a, v_b = map(float, parts)
        except (ValueError, IndexError):
            print(f"Error: Could not parse voltages for byte {byte_val:#04x}: {response}")
            failed_count += 1
            continue

        expected_v = get_expected_voltage(byte_val, switches_on)
        current_tolerance = zero_thresh_v if math.isclose(expected_v, 0.0) else tolerance_v

        fail_a = not math.isclose(v_a, expected_v, abs_tol=current_tolerance)
        fail_b = not math.isclose(v_b, expected_v, abs_tol=current_tolerance)

        if fail_a or fail_b:
            failed_count += 1
            fail_a_str = '(FAIL)' if fail_a else ''
            fail_b_str = '(FAIL)' if fail_b else ''
            print(f"-> FAIL @ {byte_val:#04x} (exp: {expected_v:.3f}V): "
                  f"Ch A={v_a:.3f}V {fail_a_str}, "
                  f"Ch B={v_b:.3f}V {fail_b_str}")
        else:
            passed_count += 1
            print(f"  OK   @ {byte_val:#04x} (exp: {expected_v:.3f}V): Ch A={v_a:.3f}V, Ch B={v_b:.3f}V")

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

