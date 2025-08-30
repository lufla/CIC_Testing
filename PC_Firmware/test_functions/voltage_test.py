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
VCAN_1_9V_CODES = {0x03, 0x07, 0x0b, 0x13, 0x17, 0x1b, 0x23, 0x27, 0x2b, 0x43, 0x47, 0x4b,
                   0x53, 0x57, 0x5b, 0x63, 0x67, 0x6b, 0x83, 0x87, 0x8b, 0x93, 0x97, 0x9b,
                   0xa3, 0xa7, 0xab}
VCAN_2_0V_CODES = {0x0f, 0x1f, 0x2f, 0x4f, 0x5f, 0x6f, 0x8f, 0x9f, 0xaf}
VCAN_2_4V_CODES = {0x33, 0x37, 0x3b, 0x73, 0x77, 0x7b, 0xb3, 0xb7, 0xbb}
VCAN_2_6V_CODES = {0x3f, 0x7f, 0xbf}
VCAN_2_8V_CODES = {0xc3, 0xc7, 0xcb, 0xd3, 0xd7, 0xdb, 0xe3, 0xe7, 0xeb}
VCAN_3_5V_CODES = {0xcf, 0xdf, 0xef}
VCAN_4_2V_CODES = {0xf3, 0xf7, 0xfb}
VCAN_4_7V_CODES = {0xff}


def get_expected_voltage(byte_value, switches_on):
    """Calculates the expected voltage based on the corrected, data-driven sets."""
    if not switches_on:
        return 1.25 if byte_value in SWITCHES_OFF_1_25V_CODES else 0.0
    else:
        if byte_value in VCAN_1_9V_CODES: return 1.9
        if byte_value in VCAN_2_0V_CODES: return 2.0
        if byte_value in VCAN_2_4V_CODES: return 2.4
        if byte_value in VCAN_2_6V_CODES: return 2.6
        if byte_value in VCAN_2_8V_CODES: return 2.8
        if byte_value in VCAN_3_5V_CODES: return 3.5
        if byte_value in VCAN_4_2V_CODES: return 4.2
        if byte_value in VCAN_4_7V_CODES: return 4.7
        return 1.25 if (byte_value & 0x03) == 0x03 else 0.0


def get_i2c_voltage(ser, channel):
    """Sends a command to read I2C voltage and parses the response."""
    prefix = f"I2C_VOLTAGE_{channel}:"
    command = f"READ_I2C_VOLTAGE_{channel}\n"
    ser.reset_input_buffer()
    ser.write(command.encode('utf-8'))
    ser.flush()
    response = ser.readline().decode('utf-8').strip()

    if not response.startswith(prefix):
        print(f"Error: Unexpected I2C response for Ch {channel}: {response}")
        return -999.0
    try:
        return float(response.split(':')[1])
    except (ValueError, IndexError):
        print(f"Error: Could not parse I2C voltage for Ch {channel}: {response}")
        return -999.0


def run_test_cycle(ser, switches_on, config, session_details, logger):
    """
    Runs through all 256 combinations, checking both SPI and I2C voltages.
    """
    print(f"\n--- Testing all 256 combinations with DIL switches {'ON' if switches_on else 'OFF'} ---")
    time.sleep(0.5)

    # Load tolerance values from config
    v_spi_tol = config['settings']['voltage_test_tolerance_v']
    zero_thresh = config['settings']['zero_threshold_v']
    high_v_tol = config['settings']['high_voltage_tolerance_v']
    v_i2c_tol = config['settings']['i2c_voltage_tolerance_v']
    i2c_high_v_tol = config['settings']['i2c_high_voltage_tolerance_v']

    passed_count = 0
    failed_count = 0

    logged_data = []

    for byte_val in range(257):
        if byte_val > 0:
            byte_to_check = byte_val - 1
            expected_v = get_expected_voltage(byte_to_check, switches_on)

            # Determine dynamic tolerance for SPI voltage
            current_spi_tol = zero_thresh if math.isclose(expected_v, 0.0) else \
                high_v_tol if expected_v > 4.0 else v_spi_tol

            # Determine dynamic tolerance for I2C voltage
            current_i2c_tol = zero_thresh if math.isclose(expected_v, 0.0) else \
                i2c_high_v_tol if expected_v > 4.0 else v_i2c_tol

            # Perform checks for both SPI and I2C channels
            fail_spi_a = not math.isclose(v_spi_a, expected_v, abs_tol=current_spi_tol)
            fail_spi_b = not math.isclose(v_spi_b, expected_v, abs_tol=current_spi_tol)
            fail_i2c_a = not math.isclose(v_i2c_a, expected_v, abs_tol=current_i2c_tol)
            fail_i2c_b = not math.isclose(v_i2c_b, expected_v, abs_tol=current_i2c_tol)

            # A channel fails if either its SPI or I2C reading is out of tolerance
            fail_a = fail_spi_a or fail_i2c_a
            fail_b = fail_spi_b or fail_i2c_b

            result_str = 'FAIL' if fail_a or fail_b else 'PASS'

            if fail_a or fail_b:
                failed_count += 1
                fstr_a = '(FAIL)' if fail_a else ''
                fstr_b = '(FAIL)' if fail_b else ''
                print(f"-> FAIL @ {byte_to_check:#04x} (exp: {expected_v:.3f}V): "
                      f"A[SPI:{v_spi_a:.3f} I2C:{v_i2c_a:.3f}]{fstr_a} | "
                      f"B[SPI:{v_spi_b:.3f} I2C:{v_i2c_b:.3f}]{fstr_b}")
            else:
                passed_count += 1
                print(f"  OK   @ {byte_to_check:#04x} (exp: {expected_v:.3f}V): "
                      f"A[SPI:{v_spi_a:.3f} I2C:{v_i2c_a:.3f}] | "
                      f"B[SPI:{v_spi_b:.3f} I2C:{v_i2c_b:.3f}]")

            # Log data for each combination
            test_data = {
                'byte_val': byte_to_check,
                'switches_on': switches_on,
                'expected_v': expected_v,
                'spi_v_a': v_spi_a,
                'spi_v_b': v_spi_b,
                'i2c_v_a': v_i2c_a,
                'i2c_v_b': v_i2c_b,
                'result': result_str
            }
            logged_data.append(test_data)

        if byte_val < 256:
            # Get SPI voltages
            ser.reset_input_buffer()
            command = f"SET_VCAN_VOLTAGE {byte_val}\n"
            ser.write(command.encode('utf-8'))
            response = ser.readline().decode('utf-8').strip()
            if response.startswith("VCAN_DATA:"):
                try:
                    parts = response.split(':')[1].split(',')
                    v_spi_a, v_spi_b = map(float, parts)
                except (ValueError, IndexError):
                    v_spi_a, v_spi_b = -998.0, -998.0
            else:
                v_spi_a, v_spi_b = -999.0, -999.0

            # Get I2C voltages for both channels
            v_i2c_a = get_i2c_voltage(ser, 'A')
            v_i2c_b = get_i2c_voltage(ser, 'B')

    print(f"\nSummary: Passed={passed_count}/256, Failed={failed_count}/256")

    # Return overall result and all logged data
    return failed_count == 0, logged_data
