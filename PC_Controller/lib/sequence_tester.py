import time
from datetime import datetime
from lib import csv_logger


def run_voltage_test(master_esp, serial_number, config):
    """Runs the voltage setting and reading test for both channels."""
    log_filename = f"CIC_QC_{serial_number}_Voltage_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    logger = csv_logger.CsvLogger(log_filename)
    logger.log_header(serial_number, config['tester_info']['operator_name'])

    voltage_levels = config['voltage_levels']
    tolerance = config['test_parameters']['voltage_tolerance_percent'] / 100.0

    for channel in ['A', 'B']: #TODO add voltage tests in parallel
        print(f"--- Testing Voltage on Channel {channel} ---")
        for voltage_str, pattern_str in voltage_levels.items():
            target_voltage = float(voltage_str)
            pattern = int(pattern_str, 16)
            
            print(f"Setting VCAN to {target_voltage}V (pattern: 0x{pattern:02X})...")
            
            cmd = {"command": "set_voltage", "channel": channel, "level": pattern}
            response = master_esp.send_command(cmd)
            print(f"  -> Device response: {response}")
            
            time.sleep(0.5)
            
            cmd = {"command": "read_voltage", "channel": channel}
            response = master_esp.send_command(cmd)
            print(f"  -> Device response: {response}")
            
            measured_voltage = response.get('data', {}).get('voltage_measured', 'N/A')
            
            status = "FAIL"
            if isinstance(measured_voltage, (float, int)):
                if abs(measured_voltage - target_voltage) <= (target_voltage * tolerance):
                    status = "PASS"

            logger.log_row(['Voltage Test', channel, f'Set {target_voltage}V', f'Measured {measured_voltage}V', status])
            time.sleep(0.5)
    print(f"Voltage test complete. Results saved to {log_filename}")

#TODO for running all, more functions are needed
def run_full_test(master_esp, serial_number, config):
    """Runs all test sequences in order.""" #TODO run voltag test with all bit sequences with OFF and then with ON, if all work and the voltages are cirrect set to 1,9V for testing the other functions
    input("Please ensure all DIL switches on the CIC are ON. Press Enter to continue...")
    run_voltage_test(master_esp, serial_number, config)
    # Add other tests here
    print("\nFull test sequence complete.")


