import time
import math
from . import voltage_test


def get_i2c_voltage(ser, channel):
    """Sends a command to read I2C voltage and parses the response."""
    prefix = f"I2C_VOLTAGE_{channel}:"
    command = f"READ_I2C_VOLTAGE_{channel}\n"
    ser.reset_input_buffer()
    ser.write(command.encode('utf-8'))
    ser.flush()
    start_time = time.time()
    while time.time() - start_time < 2.0:
        if ser.in_waiting > 0:
            response = ser.readline().decode('utf-8').strip()
            if response.startswith(prefix):
                try:
                    return float(response.split(':')[1])
                except (ValueError, IndexError):
                    print(f"Error: Could not parse I2C voltage for Ch {channel}: {response}")
                    return -999.0
    print(f"Error: Timeout waiting for I2C response for Ch {channel}")
    return -999.0


def set_current(ser, current_a, config):
    """Calculates the DAC value for a given current and sends the command."""
    settings = config['current_test_settings']
    r_ref = settings['R_REF_ohms']
    v_ref_dac = settings['V_REF_DAC_volts']
    max_possible_current = v_ref_dac / r_ref
    if current_a > max_possible_current:
        print(
            f"Warning: Requested current {current_a * 1000:.1f}mA is higher than max possible {max_possible_current * 1000:.1f}mA.")
    dac_value = (current_a * r_ref * 4095.0) / v_ref_dac
    dac_value = int(min(max(dac_value, 0), 4095))
    command = f"SET_I2C_CURRENT {dac_value}\n"
    ser.reset_input_buffer()
    ser.write(command.encode('utf-8'))
    response = ser.readline().decode('utf-8').strip()
    return response == "ACK_CURRENT_SET"


def measure_all_currents(ser):
    """Requests current readings from both master (A) and slave (B)."""
    ser.reset_input_buffer()
    ser.write(b"READ_MASTER_SPI\n")
    response_a = ser.readline().decode('utf-8').strip()
    i_a = -999.0
    if response_a.startswith("MASTER_SPI:"):
        try:
            i_a = float(response_a.split(',')[1])
        except (ValueError, IndexError):
            pass
    ser.reset_input_buffer()
    ser.write(b"CHECK_SPI_ADC\n")
    response_b = ser.readline().decode('utf-8').strip()
    i_b = -999.0
    if response_b.startswith("DATA:"):
        try:
            i_b = float(response_b.split(',')[3])
        except (ValueError, IndexError):
            pass
    return i_a, i_b


def run(ser, config, session_details, logger=None):
    """Main function to execute the current channel test, assuming pre-checks have passed."""
    print("\n" + "=" * 40)
    print("         Running Test: Current Channels")
    print("=" * 40)

    settings = config['current_test_settings']
    voltage_codes = settings['voltage_codes_for_current_test']
    target_current_a = settings['target_current_a']
    current_min_a = settings['current_min_a']
    current_max_a = settings['current_max_a']
    settle_time_s = settings['current_settle_time_s']
    v_tol = settings['voltage_tolerance_v']

    passed_count = 0
    failed_count = 0
    logged_data = []

    for code in voltage_codes:
        test_pass = True
        print(f"\n--- Testing with voltage code {code:#04x} ---")
        expected_v = voltage_test.get_expected_voltage(code, switches_on=True)

        print(f"1. Setting voltage to {expected_v:.3f}V...")
        ser.reset_input_buffer()
        ser.write(f"SET_VCAN_VOLTAGE {code}\n".encode('utf-8'))
        response = ser.readline().decode('utf-8').strip()

        v_spi_a, v_spi_b = -1.0, -1.0
        if response.startswith("VCAN_DATA:"):
            try:
                parts = response.split(':')[1].split(',')
                v_spi_a, v_spi_b = map(float, parts)
            except (ValueError, IndexError):
                pass

        v_i2c_a = get_i2c_voltage(ser, 'A')
        v_i2c_b = get_i2c_voltage(ser, 'B')

        if not (math.isclose(v_spi_a, expected_v, abs_tol=v_tol) and
                math.isclose(v_i2c_a, expected_v, abs_tol=v_tol) and
                math.isclose(v_spi_b, expected_v, abs_tol=v_tol) and
                math.isclose(v_i2c_b, expected_v, abs_tol=v_tol)):
            print(f"-> FAIL: Voltage not in range before current test. Exp: {expected_v:.3f}V")
            print(f"    A[SPI:{v_spi_a:.3f} I2C:{v_i2c_a:.3f}] | B[SPI:{v_spi_b:.3f} I2C:{v_i2c_b:.3f}]")
            failed_count += 1
            test_pass = False

            log_entry = {
                'voltage_code': code,
                'expected_v': expected_v,
                'voltage_check_pass': False,
                'v_spi_a': v_spi_a, 'v_spi_b': v_spi_b, 'v_i2c_a': v_i2c_a, 'v_i2c_b': v_i2c_b,
                'current_check_pass': False
            }
            logged_data.append(log_entry)

            continue

        print("   Voltage OK.")
        print(f"2. Setting current to {target_current_a * 1000:.1f}mA...")
        if not set_current(ser, target_current_a, config):
            print("-> FAIL: Did not receive ACK for set current command.")
            failed_count += 1
            test_pass = False

            log_entry = {
                'voltage_code': code,
                'expected_v': expected_v,
                'voltage_check_pass': True,
                'v_spi_a': v_spi_a, 'v_spi_b': v_spi_b, 'v_i2c_a': v_i2c_a, 'v_i2c_b': v_i2c_b,
                'current_set_ack': False
            }
            logged_data.append(log_entry)

            continue

        print("   Current set command sent.")
        time.sleep(settle_time_s)
        print("4. Measuring currents...")
        meas_i_a, meas_i_b = measure_all_currents(ser)

        fail_a = not (current_min_a <= meas_i_a <= current_max_a)
        fail_b = not (current_min_a <= meas_i_b <= current_max_a)

        if fail_a or fail_b:
            failed_count += 1
            fstr_a, fstr_b = ('(FAIL)' if fail_a else ''), ('(FAIL)' if fail_b else '')
            print(f"-> FAIL: Currents out of range. Exp: {current_min_a * 1000:.1f}-{current_max_a * 1000:.1f}mA")
            print(f"    A: {meas_i_a * 1000:.1f}mA {fstr_a} | B: {meas_i_b * 1000:.1f}mA {fstr_b}")
            test_pass = False
        else:
            passed_count += 1
            print(f"   Currents OK. (A: {meas_i_a * 1000:.1f}mA, B: {meas_i_b * 1000:.1f}mA)")

        # Log the current test result for this cycle
        log_entry = {
            'voltage_code': code,
            'expected_v': expected_v,
            'voltage_check_pass': True,
            'v_spi_a': v_spi_a, 'v_spi_b': v_spi_b, 'v_i2c_a': v_i2c_a, 'v_i2c_b': v_i2c_b,
            'current_set_ack': True,
            'meas_i_a': meas_i_a, 'meas_i_b': meas_i_b,
            'current_min_a': current_min_a, 'current_max_a': current_max_a,
            'result': 'PASS' if test_pass else 'FAIL'
        }
        logged_data.append(log_entry)

    set_current(ser, 0, config)
    print("\n--- Current Test Summary ---")
    print(f"Passed={passed_count}, Failed={failed_count}")

    return failed_count == 0, logged_data
