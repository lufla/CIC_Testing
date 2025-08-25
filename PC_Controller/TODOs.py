#TODO add text at all tops
#TODO generate a requirements file and a read me fot the installation of versions etc.
#TODO in the jason file switch away from default values: eg. time
#todo check that csv file is deleted, when not exited and correctly generated
#todo make on exit a pass or fail script of the excle and generate graphs , which are stored as images nex to the scv, which is in a seperate folder
#todo make scrit to push the date into the cern database

#todo SPI: ADC, M, C; I²C master and slave; Uart Master-Slave
#todo communcation and python code for current, temperature, can, crosstak




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

