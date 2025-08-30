import time
import sys

def read_all_spi_values(ser):
    """
    Reads voltage and current from both the master (A) and slave (B) via SPI.
    Returns: A tuple (v_a, i_a, v_b, i_b). Returns -999.0 for any failed reading.
    """
    v_a, i_a, v_b, i_b = -999.0, -999.0, -999.0, -999.0

    # Read from Master
    ser.reset_input_buffer()
    ser.write(b"READ_MASTER_SPI\n")
    response_a = ser.readline().decode('utf-8').strip()
    if response_a.startswith("MASTER_SPI:"):
        try:
            parts = response_a.split(':')[1].split(',')
            v_a = float(parts[0])
            i_a = float(parts[1])
        except (ValueError, IndexError):
            pass # Keep default error values

    # Read from Slave
    ser.reset_input_buffer()
    ser.write(b"CHECK_SPI_ADC\n")
    response_b = ser.readline().decode('utf-8').strip()
    if response_b.startswith("DATA:"):
        try:
            # Format is DATA:cic_v,cic_i,vcan_v,vcan_i
            parts = response_b.split(':')[1].split(',')
            v_b = float(parts[2])
            i_b = float(parts[3])
        except (ValueError, IndexError):
            pass # Keep default error values

    return v_a, i_a, v_b, i_b

def run(ser, config):
    """
    Initiates and monitors a burnout test by directly controlling and polling
    the hardware from Python.
    """
    print("\n--- Running Test: Burnout Sequence (Python-Controlled) ---")
    test_passed = False

    try:
        # --- Load Configuration ---
        burnout_cfg = config['burnout_test_settings']
        ranges = config['initial_check_ranges']

        duration_min = burnout_cfg['duration_minutes']
        duration_sec = duration_min * 60
        max_i_setting = burnout_cfg['max_i2c_dac_value'] # Direct DAC value
        max_v_setting = burnout_cfg.get('max_vcan_setting', 255) # Voltage code

        # Safety check ranges
        v_min, v_max = ranges['vcan_v_min'], ranges['vcan_v_max']
        i_min, i_max = ranges['vcan_i_min'], ranges['vcan_i_max']

    except KeyError as e:
        print(f"ERROR: Missing key in config.json: {e}")
        return False

    # --- Test Execution ---
    try:
        # 1. Set max voltage and current
        print(f"This test will run for {duration_min} minute(s).")
        print(f"Setting max voltage (code: {max_v_setting}) and max current (DAC: {max_i_setting})...")
        ser.write(f"SET_VCAN_VOLTAGE {max_v_setting}\n".encode('utf-8'))
        time.sleep(0.1)
        ser.write(f"SET_I2C_CURRENT {max_i_setting}\n".encode('utf-8'))
        time.sleep(1) # Allow time for components to settle

        # 2. Monitor for the duration
        start_time = time.time()
        end_time = start_time + duration_sec

        while time.time() < end_time:
            v_a, i_a, v_b, i_b = read_all_spi_values(ser)
            remaining_time = end_time - time.time()

            # Check for communication errors
            if -999.0 in [v_a, i_a, v_b, i_b]:
                print("\nERROR: Failed to read sensor values. Aborting test.")
                return False # Exit immediately, finally block will handle cleanup

            # Check if values are within safety ranges
            v_a_ok = v_min <= v_a <= v_max
            i_a_ok = i_min <= i_a <= i_max
            v_b_ok = v_min <= v_b <= v_max
            i_b_ok = i_min <= i_b <= i_max

            if not all([v_a_ok, i_a_ok, v_b_ok, i_b_ok]):
                print("\n--- FAILED: A measurement went out of the safe range! ---")
                print(f"    V_A: {v_a:.3f}V {'(OK)' if v_a_ok else '(FAIL)'} | I_A: {i_a*1000:.1f}mA {'(OK)' if i_a_ok else '(FAIL)'}")
                print(f"    V_B: {v_b:.3f}V {'(OK)' if v_b_ok else '(FAIL)'} | I_B: {i_b*1000:.1f}mA {'(OK)' if i_b_ok else '(FAIL)'}")
                return False # Exit immediately, finally block will handle cleanup

            # Display progress
            progress_msg = (
                f"  -> In progress... Time left: {int(remaining_time // 60)}m {int(remaining_time % 60)}s | "
                f"A(V:{v_a:.2f}, I:{i_a*1000:.1f}mA) | B(V:{v_b:.2f}, I:{i_b*1000:.1f}mA)"
            )
            sys.stdout.write('\r' + ' ' * 120) # Clear line
            sys.stdout.write('\r' + progress_msg)
            sys.stdout.flush()

            time.sleep(1) # Poll every second

        # If the loop completes without issue
        sys.stdout.write('\n') # Move to the next line after progress bar
        print("  -> Test completed successfully. All readings remained in range.")
        test_passed = True

    except KeyboardInterrupt:
        sys.stdout.write('\n')
        print("\nWARN: Burnout test interrupted by user.")
        # test_passed remains False

    finally:
        # --- Cleanup ---
        # CRITICAL: Always turn off power regardless of test outcome
        print("Cleaning up: Turning off voltage and current...")
        ser.write(b"SET_VCAN_VOLTAGE 0\n")
        time.sleep(0.1)
        ser.write(b"SET_I2C_CURRENT 0\n")

    return test_passed