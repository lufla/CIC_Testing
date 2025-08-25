import csv
from datetime import datetime
import os

class CsvLogger:
    def __init__(self, filename, directory="test_results"):
        """
        Initializes the logger. Creates a directory for logs if it doesn't exist.

        Args:
            filename (str): The name of the log file.
            directory (str): The directory where log files will be saved.
        """
        if not os.path.exists(directory):
            os.makedirs(directory)

        self.filepath = os.path.join(directory, filename)
        self.file = open(self.filepath, 'w', newline='', encoding='utf-8')
        self.writer = csv.writer(self.file)

        # Write the main header row for the data columns
        self.writer.writerow(['Test Name', 'Channel', 'Parameter', 'Measurement', 'Status', 'Timestamp'])

    def log_header(self, session_details):
        """
        Logs the overall test information at the top of the file using confirmed session details.

        Args:
            session_details (dict): A dictionary containing all the test session info.
        """
        self.writer.writerow(['Serial Number', session_details['serial_number']])
        self.writer.writerow(['Operator', session_details['operator_name']])
        self.writer.writerow(['Master ID', session_details['master_id']])
        self.writer.writerow(['Test Date', session_details['date']])
        self.writer.writerow(['Test Time', session_details['time']])
        self.writer.writerow([])  # Add a blank line for separation before the data starts

    def log_row(self, data_list):
        """
        Logs a single row of test data with a timestamp.

        Args:
            data_list (list): A list of strings representing the test data
                              (e.g., ['Voltage Test', 'A', 'Set 1.9V', 'Measured 1.88V', 'PASS']).
        """
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]  # Milliseconds
        try:
            self.writer.writerow(data_list + [timestamp])
            self.file.flush()  # Ensure data is written to the file immediately
        except Exception as e:
            print(f"Error writing to log file: {e}")

    def close(self):
        """Closes the log file."""
        if self.file:
            self.file.close()
            self.file = None

    def __del__(self):
        """Destructor to ensure the file is closed when the object is destroyed."""
        self.close()
