import time
import re


def parse_data_response(response):
    """Parses the 'DATA:' response string into a dictionary of floats."""
    if not response or not response.startswith("DATA:"):
        return None

    parts = response.replace("DATA:", "").strip().split(',')
    if len(parts) == 4:
        try:
            return {
                "cic_v": float(parts[0]),
                "cic_i": float(parts[1]),
                "vcan_v": float(parts[2]),
                "vcan_i": float(parts[3])
            }
        except (ValueError, IndexError):
            return None
    return None


def run(ser, config, ranges, session_details, logger=None, is_pre_check=False):
    """
    Performs initial hardware checks by reading sensor values and
    comparing them against expected ranges from the config file.
    """
    duration = config['settings']['initial_voltage_duration']

    if not is_pre_check:
        print("\n--- Expected Ranges ---")
        print(f"  CIC Voltage:  {ranges['cic_v_min']:.3f}V - {ranges['cic_v_max']:.3f}V  |  "
              f"CIC Current:  {ranges['cic_i_min'] * 1000:.1f}mA - {ranges['cic_i_max'] * 1000:.1f}mA")
        print(f"  VCAN Voltage: {ranges['vcan_v_min']:.3f}V - {ranges['vcan_v_max']:.3f}V  |  "
              f"VCAN Current: {ranges['vcan_i_min'] * 1000:.1f}mA - {ranges['vcan_i_max'] * 1000:.1f}mA")
        print(f"Starting check for {duration} seconds...")

    ser.reset_input_buffer()

    start_time = time.time()
    all_checks_passed = True
    readings = {}

    while time.time() - start_time < duration:
        ser.write(b"CHECK_SPI_ADC\n")
        response = ser.readline().decode('utf-8').strip()

        readings = parse_data_response(response)

        if not readings:
            print(f"  Error: Invalid response from device: '{response}'")
            all_checks_passed = False
            continue

        # Perform checks against the provided ranges
        checks = {
            "CIC V": (ranges['cic_v_min'], readings['cic_v'], ranges['cic_v_max']),
            "VCAN V": (ranges['vcan_v_min'], readings['vcan_v'], ranges['vcan_v_max']),
            "CIC I": (ranges['cic_i_min'], readings['cic_i'], ranges['cic_i_max']),
            "VCAN I": (ranges['vcan_i_min'], readings['vcan_i'], ranges['vcan_i_max'])
        }

        pass_fail_summary = []
        for name, (min_val, val, max_val) in checks.items():
            if not (min_val <= val <= max_val):
                all_checks_passed = False
                pass_fail_summary.append(f"{name} FAIL")

        if not all_checks_passed:
            print(f"  FAIL: Readings out of range. {', '.join(pass_fail_summary)}")
        else:
            print(f"  OK: CIC V:{readings['cic_v']:.3f}V, I:{readings['cic_i'] * 1000:.1f}mA | "
                  f"VCAN V:{readings['vcan_v']:.3f}V, I:{readings['vcan_i'] * 1000:.1f}mA")

        time.sleep(0.2)  # Short delay between readings

    if not is_pre_check:
        print("\n--- Initial Check Complete ---")
        print(f"Result: {'PASS' if all_checks_passed else 'FAIL'}")

    # Log the final results
    if logger and not is_pre_check:
        log_data = {
            "ranges": ranges,
            "readings": readings,
            "overall_pass": all_checks_passed
        }
        logger.log_data("Initial Checks", 'PASS' if all_checks_passed else 'FAIL', session_details, log_data)

    return all_checks_passed, readings
