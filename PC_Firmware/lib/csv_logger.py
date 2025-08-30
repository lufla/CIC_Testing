import pandas as pd
import matplotlib.pyplot as plt
import os
import sys


def analyze_and_plot_logs(log_file_path):
    """
    Analyzes the CSV log file and generates a summary and plots.
    """
    if not os.path.exists(log_file_path):
        print(f"Error: Log file not found at '{log_file_path}'.")
        return

    try:
        df = pd.read_csv(log_file_path)
    except Exception as e:
        print(f"Error: Could not read CSV file. {e}")
        return

    # --- Generate Summary Report ---
    summary_path = log_file_path.replace('.csv', '_summary.txt')
    with open(summary_path, 'w') as f:
        f.write("--- Test Session Summary ---\n")

        # Get session details from the first row
        session_data = df.iloc[0]
        f.write(f"Operator: {session_data['Operator_Name']}\n")
        f.write(f"Master ID: {session_data['Master_ID']}\n")
        f.write(f"Serial Number: {session_data['Serial_Number']}\n")
        f.write(f"Test Start: {session_data['Timestamp']}\n")
        f.write("\n--- Test Results ---\n")

        # Check the overall result from the last row of the full test sequence
        overall_result = "FAIL"
        if df['Test_Name'].str.contains('Full Test Sequence').any():
            overall_result = df[df['Test_Name'] == 'Full Test Sequence'].iloc[-1]['Overall_Result']

        f.write(f"Overall Test Sequence Result: {overall_result}\n\n")

        # Detailed results for each test
        for test_name in df['Test_Name'].unique():
            test_df = df[df['Test_Name'] == test_name]
            result = test_df['Overall_Result'].iloc[-1]
            f.write(f"- {test_name}: {result}\n")

    print(f"Analysis summary saved to: {summary_path}")

    # --- Generate Plots ---
    # Convert Test_Specific_Data from string to dictionary for easier plotting
    df['Test_Specific_Data'] = df['Test_Specific_Data'].apply(lambda x: eval(x) if isinstance(x, str) and x else {})

    # Extract readings for plotting
    readings = []
    timestamps = []

    initial_check_df = df[df['Test_Name'] == 'Initial Checks']
    if not initial_check_df.empty:
        for _, row in initial_check_df.iterrows():
            if 'readings' in row['Test_Specific_Data']:
                readings.append(row['Test_Specific_Data']['readings'])
                timestamps.append(row['Timestamp'])

    if readings:
        readings_df = pd.DataFrame(readings)
        readings_df.index = pd.to_datetime(timestamps)

        fig, ax1 = plt.subplots(figsize=(12, 6))

        # Plot Voltages on primary y-axis
        ax1.set_xlabel('Time')
        ax1.set_ylabel('Voltage (V)', color='tab:blue')
        ax1.plot(readings_df.index, readings_df['cic_v'], label='CIC Voltage', color='tab:blue')
        ax1.plot(readings_df.index, readings_df['vcan_v'], label='VCAN Voltage', color='tab:cyan')
        ax1.tick_params(axis='y', labelcolor='tab:blue')
        ax1.grid(True, which='both', linestyle='--', linewidth=0.5)

        # Create a second y-axis for Current
        ax2 = ax1.twinx()
        ax2.set_ylabel('Current (A)', color='tab:red')
        ax2.plot(readings_df.index, readings_df['cic_i'], label='CIC Current', color='tab:red', linestyle='--')
        ax2.plot(readings_df.index, readings_df['vcan_i'], label='VCAN Current', color='tab:orange', linestyle='--')
        ax2.tick_params(axis='y', labelcolor='tab:red')

        # Add title and legend
        fig.suptitle('Initial Test Voltage and Current Readings')
        fig.legend(loc="upper left", bbox_to_anchor=(0.1, 0.9))

        # Save the plot
        plot_path = log_file_path.replace('.csv', '_analysis.png')
        plt.savefig(plot_path)
        print(f"Analysis plot saved to: {plot_path}")
        plt.close(fig)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        log_file_path = sys.argv[1]
        analyze_and_plot_logs(log_file_path)
    else:
        print("Usage: python analyze_logs.py <path_to_log_file.csv>")
