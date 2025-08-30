from lib import individual_tests
import time


def run_full_test(master_esp, session_details, config):
    """
    Runs the full, sequenced QC test in the specified order.

    The sequence is:
    1.  Initial Temperature Check.
    2.  Voltage Test.
    3.  Current Test (only if Voltage Test passes).
    4.  Short CAN & Crosstalk Tests (10 messages).
    5.  Full Power Burn-in Test (heat up).
    6.  Long CAN & Crosstalk Tests (1000 messages).
    """
    print("\n--- STARTING FULL TEST SEQUENCE ---")
    # A dictionary to hold the results of each step
    test_results = {}

    # --- Step 1: Initial Temperature Test ---
    print("\n[Step 1/6] Running Initial Temperature Test...")
    temp_ok = individual_tests.run_temperature_test(master_esp, session_details, config)
    test_results['initial_temp'] = "PASS" if temp_ok else "FAIL"
    print(f"-> Initial Temperature Test Result: {test_results['initial_temp']}")
    if not temp_ok:
        print("Initial temperature is out of the acceptable range. Aborting test sequence.")
        return test_results

    # --- Step 2: Voltage Test ---
    # This test is run on both channels in parallel internally.
    print("\n[Step 2/6] Running Voltage Test...")
    voltage_ok = individual_tests.run_voltage_test(master_esp, session_details, config, 'Both')
    test_results['voltage'] = "PASS" if voltage_ok else "FAIL"
    print(f"-> Voltage Test Result: {test_results['voltage']}")

    # --- Step 3: Current Test (Conditional) ---
    # This test runs only if the voltage test was successful.
    if voltage_ok:
        print("\n[Step 3/6] Voltage OK. Running Current Test...")
        current_ok = individual_tests.run_current_test(master_esp, session_details, config, 'Both')
        test_results['current'] = "PASS" if current_ok else "FAIL"
        print(f"-> Current Test Result: {test_results['current']}")
    else:
        print("\n[Step 3/6] Skipping Current Test due to Voltage Test failure.")
        test_results['current'] = "SKIPPED"

    # Abort if critical power-related tests have failed before proceeding.
    if not voltage_ok or test_results.get('current') == "FAIL":
        print("Critical power tests failed. Aborting before burn-in and long CAN tests.")
        return test_results

    # --- Step 4: Initial CAN & Crosstalk Tests (Short) ---
    print("\n[Step 4/6] Running Short CAN Bus & Crosstalk Tests...")
    short_msg_count = config['test_parameters']['can_short_test_messages']

    can_short_a_ok = individual_tests.run_can_test(master_esp, session_details, config, 'A', short_msg_count)
    can_short_b_ok = individual_tests.run_can_test(master_esp, session_details, config, 'B', short_msg_count)
    can_short_both_ok = individual_tests.run_can_test(master_esp, session_details, config, 'Both', short_msg_count)
    crosstalk_short_ok = individual_tests.run_crosstalk_test(master_esp, session_details, config, short_msg_count)

    test_results['can_short_A'] = "PASS" if can_short_a_ok else "FAIL"
    test_results['can_short_B'] = "PASS" if can_short_b_ok else "FAIL"
    test_results['can_short_Both'] = "PASS" if can_short_both_ok else "FAIL"
    test_results['crosstalk_short'] = "PASS" if crosstalk_short_ok else "FAIL"

    print(f"-> Short CAN A: {test_results['can_short_A']}, Short CAN B: {test_results['can_short_B']}")
    print(f"-> Short CAN Both: {test_results['can_short_Both']}, Short Crosstalk: {test_results['crosstalk_short']}")

    if not all([can_short_a_ok, can_short_b_ok, can_short_both_ok, crosstalk_short_ok]):
        print("Short CAN tests failed. Aborting before burn-in and long CAN tests.")
        return test_results

    # --- Step 5: Burn-in Test (High Voltage / Current) ---
    print("\n[Step 5/6] Running Full Power Burn-In Test...")
    burn_in_ok = individual_tests.run_full_power_test(master_esp, session_details, config)
    test_results['burn_in'] = "PASS" if burn_in_ok else "FAIL"
    print(f"-> Burn-In Test Result: {test_results['burn_in']}")
    if not burn_in_ok:
        print("Burn-in test failed. Aborting remaining tests.")
        return test_results

    # --- Step 6: Main CAN & Crosstalk Tests (Long) ---
    print("\n[Step 6/6] Running Long CAN Bus & Crosstalk Tests...")
    long_msg_count = config['test_parameters']['can_long_test_messages']

    can_long_a_ok = individual_tests.run_can_test(master_esp, session_details, config, 'A', long_msg_count)
    can_long_b_ok = individual_tests.run_can_test(master_esp, session_details, config, 'B', long_msg_count)
    can_long_both_ok = individual_tests.run_can_test(master_esp, session_details, config, 'Both', long_msg_count)
    crosstalk_long_ok = individual_tests.run_crosstalk_test(master_esp, session_details, config, long_msg_count)

    test_results['can_long_A'] = "PASS" if can_long_a_ok else "FAIL"
    test_results['can_long_B'] = "PASS" if can_long_b_ok else "FAIL"
    test_results['can_long_Both'] = "PASS" if can_long_both_ok else "FAIL"
    test_results['crosstalk_long'] = "PASS" if crosstalk_long_ok else "FAIL"

    print(f"-> Long CAN A: {test_results['can_long_A']}, Long CAN B: {test_results['can_long_B']}")
    print(f"-> Long CAN Both: {test_results['can_long_Both']}, Long Crosstalk: {test_results['crosstalk_long']}")

    print("\n--- FULL TEST SEQUENCE COMPLETE ---")
    return test_results
