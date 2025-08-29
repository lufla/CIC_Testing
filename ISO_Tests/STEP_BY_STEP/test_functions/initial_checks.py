import time


def check_value(name, value, v_min, v_max):
    """Checks if a single value is within its range. Returns True if failed."""
    if not (v_min <= value <= v_max):
        print(f"-> FAILED! {name}: {value:.4f} is outside range ({v_min:.4f} - {v_max:.4f})")
        return True
    return False


def run(ser, config, ranges):
    """
    Performs the timed check of initial ADC values from the slave.
    Returns True on PASS, False on FAIL.
    """
    # Use the new key from config.json
    duration = config['settings']['initial_voltage_duration']

    print(f"\n--- Expected Ranges ---")
    print(f"  CIC Voltage:  {ranges['cic_v_min']:.3f}V - {ranges['cic_v_max']:.3f}V")
    print(f"  CIC Current:  {ranges['cic_i_min'] * 1000:.1f}mA - {ranges['cic_i_max'] * 1000:.1f}mA")
    print(f"  VCAN Voltage: {ranges['vcan_v_min']:.3f}V - {ranges['vcan_v_max']:.3f}V")
    print(f"  VCAN Current: {ranges['vcan_i_min'] * 1000:.1f}mA - {ranges['vcan_i_max'] * 1000:.1f}mA")

    print(f"\nStarting check for {duration} seconds...")
    ser.write(b'CHECK_SPI_ADC\n')
    start_time = time.time()
    test_passed = True
    data_received = False

    while time.time() - start_time < duration:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith("DATA:"):
                data_received = True
                try:
                    parts = line.split(':')[1].split(',')
                    cic_v, cic_i, vcan_v, vcan_i = map(float, parts)
                    print(
                        f"  OK: CIC V:{cic_v:.3f}V, I:{cic_i * 1000:.1f}mA | VCAN V:{vcan_v:.3f}V, I:{vcan_i * 1000:.1f}mA")

                    failure = any([
                        check_value("CIC Voltage", cic_v, ranges['cic_v_min'], ranges['cic_v_max']),
                        check_value("CIC Current", cic_i, ranges['cic_i_min'], ranges['cic_i_max']),
                        check_value("VCAN Voltage", vcan_v, ranges['vcan_v_min'], ranges['vcan_v_max']),
                        check_value("VCAN Current", vcan_i, ranges['vcan_i_min'], ranges['vcan_i_max'])
                    ])

                    if failure:
                        test_passed = False
                        break
                except (ValueError, IndexError):
                    print(f"Warning: Could not parse data: {line}")
        time.sleep(0.1)

    print("\n--- Initial Check Complete ---")
    if not data_received:
        print("Result: FAILED - No data received from slave.")
        return False

    print(f"Result: {'PASS' if test_passed else 'FAILED'}")
    return test_passed

