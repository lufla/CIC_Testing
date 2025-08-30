import time
import sys


def run(ser, config):
    """
    Initiates and monitors a long-duration burnout test with on-device safety checks.
    """
    print("\n--- Running Test: Burnout Sequence ---")

    try:
        burnout_cfg = config['burnout_test_settings']
        voltage_cfg = config.get('voltage_test_settings', {})
        ranges = config['initial_check_ranges']

        duration_min = burnout_cfg['duration_minutes']
        duration_sec = duration_min * 60
        max_i_setting = burnout_cfg['max_i2c_dac_value']

        # Determine the max voltage setting code
        max_v_setting = burnout_cfg.get('max_vcan_setting', 255)
        if 'voltage_codes_for_vcan_test' in voltage_cfg:
            max_v_setting = max(voltage_cfg.get('voltage_codes_for_vcan_test', [255]))

        # Get the safety check ranges from the config
        v_min = ranges['vcan_v_min']
        v_max = ranges['vcan_v_max']
        i_min = ranges['vcan_i_min']
        i_max = ranges['vcan_i_max']

    except KeyError as e:
        print(f"ERROR: Missing key in config.json: {e}")
        return False

    print(f"This test will run for {duration_min} minutes with on-device safety monitoring.")
    print(f"Test started at: {time.strftime('%H:%M')}")


    # Construct the new command with all parameters
    command = f"RUN_BURNOUT_TEST {duration_sec} {max_v_setting} {max_i_setting} {v_min} {v_max} {i_min} {i_max}\n"
    ser.write(command.encode('utf-8'))

    start_time = time.time()
    end_time = start_time + duration_sec + 10  # Add 10s buffer for firmware overhead

    try:
        while time.time() < end_time:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if not line: continue

                if "BURNOUT_PROGRESS" in line:
                    remaining_time = end_time - time.time()
                    progress_msg = f"  -> In progress... Time left: {int(remaining_time // 60)}m {int(remaining_time % 60)}s | {line.split(':', 1)[1]}"
                    sys.stdout.write('\r' + ' ' * 150)
                    sys.stdout.write('\r' + progress_msg)
                    sys.stdout.flush()

                elif "BURNOUT_FINAL:COMPLETE" in line:
                    sys.stdout.write('\n')
                    print("  -> Firmware reports burnout test complete and all values remained in range.")
                    return True

                elif "BURNOUT_FINAL:FAIL" in line:
                    sys.stdout.write('\n')
                    print(f"ERROR: Firmware reported a failure: {line.split(':', 2)[2]}")
                    return False

            time.sleep(0.1)

    except KeyboardInterrupt:
        sys.stdout.write('\n')
        print("\nWARN: Burnout test interrupted. Sending stop command...")
        ser.write(b"RUN_BURNOUT_TEST 0 0 0 0 0 0 0\n")
        time.sleep(1)
        return False

    sys.stdout.write('\n')
    print("--- FAILED (Timeout: Did not receive completion message from firmware) ---")
    return False

