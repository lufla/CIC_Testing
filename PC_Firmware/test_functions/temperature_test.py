import time


def run(ser, config, session_details, logger=None):
    """
    Commands the master and slave to read their temperature sensors
    and reports the values. A failure on the slave side is reported
    but does not fail the overall non-critical test.
    """
    print("\n--- Running Test: Temperature Communication ---")

    # Clear any old data in the input buffer
    ser.reset_input_buffer()

    # Send the command to the master ESP32
    ser.write(b"READ_TEMP\n")

    start_time = time.time()
    response_received = False
    master_temp, slave_temp = -99.9, -99.9
    test_result = 'FAIL'

    while time.time() - start_time < 5:  # 5-second timeout
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            print(f"Received: {line}")  # Debug print
            if line.startswith("TEMPERATURES:"):
                try:
                    # Example line: "TEMPERATURES:Master=24.50,Slave=25.12"
                    parts = line.split(':')[1].split(',')
                    master_temp_str = parts[0].split('=')[1]
                    slave_temp_str = parts[1].split('=')[1]

                    master_temp = float(master_temp_str)
                    slave_temp = float(slave_temp_str)

                    print(f"  Master Temperature: {master_temp:.2f} °C")

                    if slave_temp == 99.00:
                        print("  Slave Temperature: READ FAIL (Device returned 99.00)")
                        test_result = 'PARTIAL_PASS'
                    else:
                        print(f"  Slave Temperature: {slave_temp:.2f} °C")
                        test_result = 'PASS'

                    response_received = True
                    break  # Exit the loop once we have our data
                except (IndexError, ValueError) as e:
                    print(f"  FAIL: Could not parse response string: '{line}'. Error: {e}")
                    # Continue waiting for a correctly formatted line

    if not response_received:
        print("  FAIL: No valid 'TEMPERATURES' response from master device.")
        test_result = 'FAIL'

    # Log the temperature data
    if logger:
        log_data = {
            'master_temp': master_temp,
            'slave_temp': slave_temp
        }
        logger.log_data("Temperature Communication", test_result, session_details, log_data)

    # Per user request, this is not a critical test, so we don't return False.
    # We simply report the findings.
    return True, log_data
