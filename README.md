# ESP32-QC-Tester

This repository provides the firmware and PC-side software for a Quality Control (QC) test station designed to validate the CIC board fot the ATLAS-ITK MOPS-Hub hardware. The system performs a series of electrical and communication tests, logs all data to a CSV file, and generates graphical summaries for quick analysis.

---

## Features

* **Automated Test Suite**: Runs a comprehensive sequence of tests, including initial checks, voltage, current, temperature, and CAN communication.
* **Configurable Settings**: All test parameters, tolerances, and hardware configurations are managed via a `config.json` file.
* **Persistent Logging**: Automatically generates a timestamped CSV log file for every test session, storing all measured values, ranges, and test results in a dedicated `logs` directory.
* **Automated Analysis**: A utility script generates graphical representations of the test data (e.g., voltage and current measurements) and a summary report upon script exit.
* **Clear Hardware Manual**: Includes detailed instructions for physical setup and connections.
* **Flexible Vref/Rref**: The DAC reference voltage (`V_REF_DAC_volts`) and current reference resistor (`R_REF_ohms`) are configurable in `config.json`.

---

## Prerequisites

### Hardware

* Two ESP32 development boards (one configured as Master, one as Slave)
* PC with a USB port (for each ESP32, if using multiple testers)
* Programmable lab power supply
* Multimeter
* Relevant cables for serial and power connections

### Software

* Python 3.9
* Arduino IDE with the ESP32 board package installed
* All Python dependencies listed in `requirements.txt`
* All required Arduino libraries for the firmware (assumed to be included locally in the `lib` folder of the firmware project, as a best practice).
* **Datasheets**: All datasheets for the components used in the hardware design should be added to this repository for easy reference.

---

## Installation and Setup

### 1. Software Installation

1.  Clone this repository to your local machine:
    ```bash
    git clone [TODO]
    cd ESP32-QC-Tester
    ```

2.  Install the required Python packages using the provided `requirements.txt` file:
    ```bash
    pip install -r requirements.txt
    ```

### 2. Hardware Assembly and Wiring Manual

This section is critical for correct operation. Follow these steps precisely.

**Lab Power Supply**:

* Connect the GND of the VCAN power supply and the GND of the CIC power supply together.
* Connect both of these power supply GNDs to the Earth ground via the power supply's connectors. This is crucial for safety and accurate readings.
* Set the power supply voltage as requested by the Python script at the beginning of the test.

**ESP32s**:

* Wire the Master and Slave ESP32s according to the provided schematics.
* For setups with more than one tester, it is critical to use USB isolators for each ESP32 to prevent ground loops. This isolates the PC's ground from the test hardware's ground.

**Firmware Upload**:

* Open the `main.ino` file in the Arduino IDE.
* Ensure all required libraries are present in the `src` folder (e.g., `lib/temperature_handler.h`, `lib/spi_handler`, etc.).
* Connect the Master ESP32 to your PC. Select the correct board and COM port, then upload the firmware.
* Repeat the process for the Slave ESP32.

---

## Configuration

The `config.json` file is the central point for all test parameters.

* `tester_info`:
    * `master_id`: A unique identifier for your tester board.
    * `lab_power_supply_voltage_v`: The expected voltage from your lab power supply. This value is used by the firmware's ADC for accurate readings and is asked for at the start of the script.
* `current_test_settings`:
    * `V_REF_DAC_volts`: The reference voltage of the DAC, which is crucial for current consumption calculations.
    * `R_REF_ohms`: The reference resistance value.
* `initial_check_ranges`: Define the minimum and maximum acceptable values for initial voltage and current readings.
* `can_test_settings`: Configures the CAN communication test, including the number of messages for short and long runs.
* `burnout_test_settings`: Configures the optional burnout test. This test is a separate step and should only be performed after the initial voltage tests have passed. The full test sequence in `main.py` is configured to enforce this.

---

## Usage

1.  Ensure all hardware is wired correctly and the firmware is uploaded.
2.  Connect the Master ESP32 to your PC.
3.  Run the Python script from your terminal:
    ```bash
    python main.py
    ```
4.  Follow the on-screen prompts to enter the operator name, serial number, and lab power supply voltage.
5.  Select an option from the main menu:
    * **1. Start Full Test Sequence**: This will automatically run a comprehensive set of tests. The burnout test is included in this sequence and will only proceed after the critical voltage and current tests have passed.
    * **2-7**: Run individual tests.
    * **8. Exit**: Safely exit the program.

---

## Log Files and Data Analysis

Upon program exit, a new folder named `logs` is created in the same directory as `main.py`. This folder contains:

* `test_log_[timestamp].csv`: The primary log file with all test data. The script ensures this file is properly closed and saved even if an error occurs.
* `analysis_[timestamp].png`: A graph showing voltage and current readings over time.
* `summary_[timestamp].txt`: A simple text file with a Pass/Fail summary of the full test sequence.