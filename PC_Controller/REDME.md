# CIC Quality Control Test Suite

This project contains the software and firmware for testing the CIC (Custom Interface Card). It consists of a Python application that runs on a PC and communicates with ESP32-based firmware on a master/slave test jig.

## Features

-   Automated test sequences for various hardware components.
-   Interactive command-line interface for selecting tests.
-   Serial communication with the test hardware.
-   Logging of test results to CSV files.
-   Automatic detection of the Test Station ID from the hardware.

## Project Structure


.
├── lib/
│   ├── serial_handler.py   # Handles serial port detection and communication
│   ├── sequence_tester.py  # Defines the test sequences
│   └── csv_logger.py       # Handles logging results to CSV
├── test_results/           # Directory where CSV logs are saved
├── main.py                 # Main entry point for the Python application
├── config.jason            # Configuration file for test parameters
└── requirements.txt        # Python package dependencies


## Python Application Setup

### Prerequisites

-   Python 3.7 or newer
-   A text editor or IDE (e.g., VS Code)

### Installation

1.  **Clone the repository:**
    ```bash
    git clone <your-repository-url>
    cd <your-repository-directory>
    ```

2.  **Create a virtual environment (recommended):**
    ```bash
    python -m venv venv
    ```

3.  **Activate the virtual environment:**
    -   On Windows:
        ```bash
        .\venv\Scripts\activate
        ```
    -   On macOS/Linux:
        ```bash
        source venv/bin/activate
        ```

4.  **Install the required Python packages:**
    ```bash
    pip install -r requirements.txt
    ```

### Running the Application

1.  Ensure the Master ESP32 of the test jig is connected to the PC via USB.
2.  Run the `main.py` script from your terminal:
    ```bash
    python main.py
    ```
3.  Follow the on-screen prompts to select the serial port, confirm test details, and run the desired tests.

## ESP32 Firmware Setup

For instructions on how to set up the Arduino IDE, install libraries, and flash the firmware, please refer to the comments at the top of the `QC_Tester_Firmware.ino` file.
