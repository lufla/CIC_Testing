#todo fist test should be testing the SPI_ADC that the CIC is powered correctly
#test 256 combinations and check if they are (see matrix) 0, 1.25; 1.9;... when desiired (see matrix again). If all of this is okay for OFF and ON, only then persute with every other test
#TODO add text at all tops
#TODO generate a requirements file and a read me fot the installation of versions etc.
#TODO in the jason file switch away from default values: eg. time
#todo check that csv file is deleted, when not exited and correctly generated
#todo make on exit a pass or fail script of the excle and generate graphs , which are stored as images nex to the scv, which is in a seperate folder
#todo make scrit to push the date into the cern database
#todo add datasheets to the repo, as well as add lib files localy of the arduino part.
#todo make also jason files for Arduino sheeld & ESP32
#9 min test only after voltage tests have been permed
#todo to use more then one setup, GND has to be isolated. So GND of the PC and the powersupply
#todo QC tester board test: If everything is inside the range when connected to a CIC (best case a validated CIC), everything is fine. Not possible for the fist QC board of its cind
#todo define sampling rate of each value
#todo for current consuption v_ref and r_ref have to be in the jason file

#todo make the sending of commandes easier, because sometimes the sent  just needs to act as a button

#// --- Simulated Input Conditions ---
#TODO Known VCAN input voltage for verification.  ADD THIS TOD THE PYTHON CODE FOR EVRIFICATION ON THE INITIAL TEST
#  const float QC_VCAN_SUPPLY_VOLTAGE = 9.0;

#todo add calculations to put the values which re saved into the correct state, becasue
# eg. the SPI_ADC uses resistors to convert the current and voltage, they have to be changed back mathematicaly
# after hardware sets the values expeted are:
# CIC power:      	 	 Voltage = 4.95 V
# CIC power:      	 	 Current = 0.01 A
# VCAN before CIC ADC: 	 	 Voltage = 1.12 V
# VCAN before CIC ADC: 	 	 Current = 0.05 A
# they have to be reanslated back

#todo SPI: ADC, M, C; I²C master and slave; Uart Master-Slave
#todo communcation and python code for current, temperature, can, crosstak


#todo in the maual connect the GND of VCAN_PSU and CIC_Power togther, but with a cable connecting both together in the powersupply connectors

#todo call all functions when needed:
#start taking the temperature,
#voltage test parallel,
#current test parallel (only if voltage is okay),
#CAN test short (10 msg only to check if it works in general),
#for CAN do A, B and both,
#then crosstalk with 10msg,
#if all looks fine with sent and receive as expected,
#heat up for 9 min with high voltage, high current,
#then make 1000msg channel A, then 1000 B, then 1000 A and B,
#thenn 1000 crosstalk test (this can be integrated into the CAN part)

