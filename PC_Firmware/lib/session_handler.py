import re


def get_and_confirm_details(config):
    """
    Prompts the user for session details (operator name, serial number, power supply voltage)
    and validates them.
    """
    print("Please enter the test session details.")

    # Get operator name
    operator_name = input(f"Operator Name (default: {config['tester_info']['operator_name']}): ")
    if not operator_name:
        operator_name = config['tester_info']['operator_name']

    # Get serial number
    serial_number = input("Device Serial Number: ")
    validation_rules = config['tester_info']['validation_rules']

    # Validate serial number
    if len(serial_number) != validation_rules.get('serial_number_length', 0):
        print(f"Error: Serial number must be {validation_rules['serial_number_length']} characters.")
        return None
    if validation_rules.get('serial_number_numeric_only', False) and not serial_number.isdigit():
        print("Error: Serial number must contain only numbers.")
        return None

    # Get lab power supply voltage
    default_voltage = config['tester_info']['lab_power_supply_voltage_v']
    try:
        power_supply_voltage_str = input(f"Lab Power Supply Voltage in V (default: {default_voltage}V): ")
        if not power_supply_voltage_str:
            power_supply_voltage = default_voltage
        else:
            power_supply_voltage = float(power_supply_voltage_str)
    except ValueError:
        print("Error: Invalid voltage value. Please enter a number.")
        return None

    print("\n--- Details Confirmation ---")
    print(f"Operator: {operator_name}")
    print(f"Serial Number: {serial_number}")
    print(f"Lab Power Supply Voltage: {power_supply_voltage}V")

    confirm = input("Are these details correct? (y/n): ").lower()
    if confirm == 'y':
        return {
            'operator_name': operator_name,
            'serial_number': serial_number,
            'lab_power_supply_voltage_v': power_supply_voltage
        }
    else:
        return None
