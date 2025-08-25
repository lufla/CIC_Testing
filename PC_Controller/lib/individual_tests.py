import time
from datetime import datetime
from lib import csv_logger


# --- HELPER FUNCTIONS ---

def _get_expected_voltage(pattern, valid_patterns_map):
    """
    Determines the expected voltage based on the pattern.
    - Power is only enabled if bits 0 and 1 are both 1 (pattern & 0x03 == 0x03).
    - Otherwise, the expected voltage is 0.0.
    """
    if (pattern & 0x03) != 0x03:
        return 0.0
    return valid_patterns_map.get(pattern, None)


# --- VOLTAGE TEST ---

def run_voltage_test(master_esp, session_details, config, channel_choice):
    """
    Runs a comprehensive voltage test and returns True if all checks pass, otherwise False.
    """
    serial_number = session_details['serial_number']
    logger = csv_logger.CsvLogger(
        f"CIC_QC_{serial_number}_Voltage_Exhaustive_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")
    logger.log_header(session_details)

    test_passed = True  # Flag to track the overall result of this test
    valid_patterns = {int(v, 16): float(k) for k, v in config['valid_voltage_patterns'].items()}
    tolerance = config['test_parameters']['voltage_tolerance_percent'] / 100.0
    channels = ['A', 'B'] if channel_choice == 'Both' else [channel_choice]

    # --- PHASE 1: DIL SWITCHES OFF ---
    input(
        "\n--- VOLTAGE TEST PHASE 1: SWITCHES OFF ---\nPlease ensure all DIL switches on the CIC are OFF. Press Enter to begin...")
    print("Cycling through all 256 bit patterns with switches OFF...")
    for pattern in range(256):
        for channel in channels:
            master_esp.send_command({"command": "set_voltage", "channel": channel, "level": pattern})
            logger.log_row(
                ['Voltage Test (Switches OFF)', channel, f'Set Pattern 0x{pattern:02X}', 'N/A', 'ACTION_LOGGED'])
    print("Phase 1 complete.")

    # --- PHASE 2: DIL SWITCHES ON ---
    input(
        "\n--- VOLTAGE TEST PHASE 2: SWITCHES ON ---\nPlease ensure all DIL switches on the CIC are ON. Press Enter to begin...")
    print("Cycling through all 256 bit patterns with switches ON for verification...")
    for pattern in range(256):
        for channel in channels:
            master_esp.send_command({"command": "set_voltage", "channel": channel, "level": pattern})
            time.sleep(0.05)  # Brief pause for voltage to stabilize
            response = master_esp.send_command({"command": "read_voltage", "channel": channel})

            measured_voltage = response.get('data', {}).get('voltage_measured', 'N/A')
            expected_voltage = _get_expected_voltage(pattern, valid_patterns)

            status = "FAIL"
            if expected_voltage is None:
                status = "UNDEFINED_PATTERN"
            elif isinstance(measured_voltage, (float, int)):
                if expected_voltage == 0.0 and abs(measured_voltage) < 0.1:
                    status = "PASS"
                elif expected_voltage > 0.0 and abs(measured_voltage - expected_voltage) <= (
                        expected_voltage * tolerance):
                    status = "PASS"

            if status == "FAIL":
                test_passed = False  # If any single measurement fails, the entire test fails

            logger.log_row(['Voltage Test (Switches ON)', channel, f'Set Pattern 0x{pattern:02X}',
                            f'Measured {measured_voltage}V (Expected ~{expected_voltage}V)', status])

    print("Phase 2 complete.")
    logger.close()
    print("--- Voltage Test Complete ---")
    return test_passed


# --- FULL POWER (BURN-IN) TEST ---

def run_full_power_test(master_esp, session_details, config):
    """
    Runs a full power burn-in test and returns True on success.
    Success is defined as the final voltage being within tolerance after the duration.
    """
    serial_number = session_details['serial_number']
    logger = csv_logger.CsvLogger(f"CIC_QC_{serial_number}_FullPower_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")
    logger.log_header(session_details)

    test_passed = True
    duration = config['test_parameters'].get('full_power_duration_seconds', 10)
    print(f"\n--- Starting Full Power Burn-In Test for {duration} seconds ---")
    logger.log_row(['Full Power Test', 'Both', 'Starting Test', f'Duration: {duration}s', 'INFO'])

    # Set highest voltage (0xFF) and max current
    master_esp.send_command({"command": "set_voltage", "channel": "A", "level": 0xFF})
    master_esp.send_command({"command": "set_voltage", "channel": "B", "level": 0xFF})
    master_esp.send_command({"command": "set_current", "channel": "A", "level": 0xFF})
    master_esp.send_command({"command": "set_current", "channel": "B", "level": 0xFF})

    print(f"Burn-in running for {duration} seconds...")
    time.sleep(duration)
    print("Burn-in complete. Checking final voltages...")

    # Assumes a command to read both voltages at once for efficiency
    response = master_esp.send_command({"command": "read_all_voltages"})
    voltage_a = response.get('data', {}).get('voltage_a', 'N/A')
    voltage_b = response.get('data', {}).get('voltage_b', 'N/A')

    # Verify final voltages are within tolerance of the expected maximum
    valid_patterns = {int(v, 16): float(k) for k, v in config['valid_voltage_patterns'].items()}
    expected_max_voltage = _get_expected_voltage(0xFF, valid_patterns)
    tolerance = config['test_parameters']['voltage_tolerance_percent'] / 100.0

    for v_str, voltage in [('A', voltage_a), ('B', voltage_b)]:
        if not isinstance(voltage, (float, int)) or \
                abs(voltage - expected_max_voltage) > (expected_max_voltage * tolerance):
            test_passed = False
            logger.log_row(
                ['Full Power Test', v_str, 'Final VCheck', f'Measured {voltage}V (Expected ~{expected_max_voltage}V)',
                 'FAIL'])
        else:
            logger.log_row(
                ['Full Power Test', v_str, 'Final VCheck', f'Measured {voltage}V (Expected ~{expected_max_voltage}V)',
                 'PASS'])

    # Power down all channels
    master_esp.send_command({"command": "set_voltage", "channel": "A", "level": 0x00})
    master_esp.send_command({"command": "set_voltage", "channel": "B", "level": 0x00})
    master_esp.send_command({"command": "set_current", "channel": "A", "level": 0x00})
    master_esp.send_command({"command": "set_current", "channel": "B", "level": 0x00})

    logger.close()
    print("--- Full Power Test Complete ---")
    return test_passed


# --- CURRENT TEST ---

def run_current_test(master_esp, session_details, config, channel_choice):
    """
    Measures current draw on the specified channels and returns True on success.
    NOTE: This is a simplified implementation based on assumed firmware commands.
    """
    print(f"\n--- Starting Current Test for Channel(s): {channel_choice} ---")
    logger = csv_logger.CsvLogger(
        f"CIC_QC_{session_details['serial_number']}_Current_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")
    logger.log_header(session_details)

    test_passed = True
    channels = ['A', 'B'] if channel_choice == 'Both' else [channel_choice]
    expected_current = config['test_parameters'].get('expected_current_ma', 100)
    tolerance = config['test_parameters']['current_tolerance_percent'] / 100.0

    for channel in channels:
        # Set a mid-range voltage and current to get a measurable value
        master_esp.send_command({"command": "set_voltage", "channel": channel, "level": 0x33})  # 2.4V
        master_esp.send_command({"command": "set_current", "channel": channel, "level": 0x80})  # 50%
        time.sleep(0.1)

        response = master_esp.send_command({"command": "read_current", "channel": channel})
        measured_current = response.get('data', {}).get('current_measured', 'N/A')

        status = "FAIL"
        if isinstance(measured_current, (float, int)):
            if abs(measured_current - expected_current) <= (expected_current * tolerance):
                status = "PASS"

        if status == "FAIL":
            test_passed = False

        logger.log_row(
            ['Current Test', channel, 'Measure', f'Measured {measured_current}mA (Expected ~{expected_current}mA)',
             status])
        master_esp.send_command({"command": "set_voltage", "channel": channel, "level": 0x00})
        master_esp.send_command({"command": "set_current", "channel": channel, "level": 0x00})

    logger.close()
    print("--- Current Test Complete ---")
    return test_passed


# --- CAN BUS TEST ---

def run_can_test(master_esp, session_details, config, channel_choice, num_messages):
    """
    Runs a CAN message send/receive test and returns True on success.
    NOTE: Assumes firmware has a command that sends N messages and returns a report.
    """
    print(f"\n--- Starting CAN Test ({num_messages} msgs) for Channel(s): {channel_choice} ---")
    cmd = {"command": "run_can_test", "channel": channel_choice, "messages": num_messages}
    response = master_esp.send_command(cmd)

    data = response.get('data', {})
    sent = data.get('messages_sent', 0)
    received = data.get('messages_received', 0)
    errors = data.get('error_frames', 0)

    # Test passes if all sent messages were received and there were no error frames
    test_passed = (sent == num_messages and received == num_messages and errors == 0)
    print(f"Result: Sent={sent}, Received={received}, Errors={errors}. Pass={test_passed}")
    return test_passed


# --- CROSSTALK TEST ---

def run_crosstalk_test(master_esp, session_details, config, num_messages):
    """
    Runs a CAN crosstalk test and returns True on success.
    NOTE: Assumes firmware can send on A while listening on B, and vice-versa.
    """
    print(f"\n--- Starting Crosstalk Test ({num_messages} msgs) ---")
    cmd = {"command": "run_crosstalk_test", "messages": num_messages}
    response = master_esp.send_command(cmd)

    data = response.get('data', {})
    # Report for sending on A, listening for bleed on B
    sent_a = data.get('sent_a', 0)
    received_b = data.get('received_on_b', 0)
    # Report for sending on B, listening for bleed on A
    sent_b = data.get('sent_b', 0)
    received_a = data.get('received_on_a', 0)

    # Test passes if messages were sent but ZERO messages were received on the other channel
    a_to_b_ok = (sent_a == num_messages and received_b == 0)
    b_to_a_ok = (sent_b == num_messages and received_a == 0)
    test_passed = a_to_b_ok and b_to_a_ok
    print(f"Result A->B: Sent={sent_a}, Received={received_b}. Pass={a_to_b_ok}")
    print(f"Result B->A: Sent={sent_b}, Received={received_a}. Pass={b_to_a_ok}")
    return test_passed


# --- TEMPERATURE TEST ---

def run_temperature_test(master_esp, session_details, config):
    """
    Reads the onboard temperature and returns True if it's within the configured range.
    """
    print("\n--- Starting Temperature Test ---")
    min_temp = config['test_parameters'].get('min_temp_c', 15.0)
    max_temp = config['test_parameters'].get('max_temp_c', 40.0)

    response = master_esp.send_command({"command": "read_temperature"})
    measured_temp = response.get('data', {}).get('temperature_c', 'N/A')

    test_passed = False
    if isinstance(measured_temp, (float, int)):
        if min_temp <= measured_temp <= max_temp:
            test_passed = True

    print(f"Result: Measured {measured_temp}°C (Range: {min_temp}-{max_temp}°C). Pass={test_passed}")
    return test_passed
