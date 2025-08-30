import time


def run(ser, config, session_details, logger=None, num_messages=None):
    """
    Initiates and verifies a two-way CAN communication test.
    """
    try:
        # If num_messages isn't passed, default to the short run from config
        if num_messages is None:
            num_messages = config['can_test_settings']['short_run_messages']

        # Calculate a dynamic timeout: 5s base + 100ms per message
        timeout_s = 5 + (num_messages * 0.1)

    except KeyError as e:
        print(f"ERROR: Missing key in 'can_test_settings' in config.json: {e}")
        return False, {}

    command = f"RUN_CAN_TEST {num_messages}\n"
    print(f"Sending command to test with {num_messages} messages (timeout: {int(timeout_s)}s)...")
    ser.write(command.encode('utf-8'))

    start_time = time.time()
    test_passed = False

    while time.time() - start_time < timeout_s:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if not line:
                continue

            if "CAN_TEST_PROGRESS" in line:
                print(f"  -> {line.split(': ', 1)[1]}")

            elif "CAN_TEST_FINAL:PASS" in line:
                print("  -> Firmware reports PASS.")
                print(f"  -> Details: {line.split(':', 2)[2]}")
                test_passed = True
                break

            elif "CAN_TEST_FINAL:FAIL" in line:
                print("  -> Firmware reports FAIL.")
                print(f"  -> Details: {line.split(':', 2)[2]}")
                break

    if not test_passed:
        print("--- FAILED (Timeout: Did not receive final status from firmware) ---")

    # Log the final result
    log_data = {
        'num_messages': num_messages,
        'timeout_s': timeout_s,
        'final_firmware_response': line if 'line' in locals() else 'TIMEOUT'
    }

    if logger:
        logger.log_data("CAN Communication", 'PASS' if test_passed else 'FAIL', session_details, log_data)

    return test_passed, log_data
