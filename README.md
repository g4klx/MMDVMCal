# MMDVMCal_freqSweep_eeprom
Stock MMDVMCal with frequency sweep test for DMR added to find optimal Rx offset for modem board and support for onboard EEPROM module for storing data

## Description
Additional features built onto MMDVMCal including:
- A automatic frequency sweep test to find optimal offset for minimum BER on DMR
- An option to store the test result data from the last run into EEPROM (Intended for simplex hotspots, writes both Tx and Rx offsets)
- Options to store manually adjusted offset into EEPROM (Intended for duplex operation, separate Tx and Rx offset writing options)
- Options to store manually adjusted offset into MMDVMHost configuration file (default Pi-Star location /etc/mmdvmhost)
- Option to write stored EEPROM data to MMDVMHost configuration file
- Nextion screen indication (Displays the text "Calibration Mode" and displays current frequency and relative offset in Tx/Rx fields)

## Usage
Additional commands added to MMDVMCal:
- S : DMR Frequency sweep test - Hold the PTT button on your radio so that it continuously transmits on the selected frequency, (CC1) until the test completes. Use 'W' to store the test result offset value into EEPROM.
- I : Initialize EEPROM - Warning, this erases stored EEPROM data and sets it to default values. Any previously stored calibration data will be permanantly erased!
- W : Store the last DMR frequency sweep test result value into EEPROM. 
- X : Store manually selected current offset to Tx offset data in EEPROM
- x : Store manually selected current offset to Rx offset data in EEPROM
- Z : Display data stored in EEPROM
- U : Write manually selected offset to Tx and Rx offset fields in MMDVMHost configuration file (Simplex)
- u : Write manually selected offset to Rx offset field in MMDVMHost configuration file
- v : Write manually selected offset to Tx offset field in MMDVMHost configuration file
- E : Write stored EEPROM offset data to MMDVMHost configuration file

### Example (Ideal for a simplex hotspot):
1. Start program
2. Press 'S' and immediately press and hold the PTT button on your radio. Do not release until the test is complete.
3. If satisfied with the test result, press 'W' to write test data to EEPROM.
4. Press 'E' to write the stored data to MMDVMHost configuration file
