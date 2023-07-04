/*
 *   Copyright (C) 2015,2016,2017,2020,2021 by Jonathan Naylor G4KLX
 *   Copyright (C) 2017,2018 by Andy Uribe CA6JAU
 *   Copyright (C) 2018 by Bryan Biedenkapp N2PLL
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(MMDVMCAL_H)
#define	MMDVMCAL_H

#include "SerialController.h"
#include "Console.h"
#include "BERCal.h"
#include "EEPROMData.h"
#include "ConfigFile.h"

#include <cstring>
#include <cstdlib>

#define VHF_FREQUENCY_MIN	144000000
#define VHF_FREQUENCY_MAX	146000000
#define UHF_FREQUENCY_MIN	420000000
#define UHF_FREQUENCY_MAX	450000000

enum RESP_TYPE_MMDVM {
	RTM_OK,
	RTM_TIMEOUT,
	RTM_ERROR
};

enum HW_TYPE {
	HWT_MMDVM,
	HWT_MMDVM_HS
};

enum MMDVM_STATE {
  STATE_IDLE      = 0,
  STATE_DSTAR     = 1,
  STATE_DMR       = 2,
  STATE_YSF       = 3,
  STATE_P25       = 4,
  STATE_NXDN      = 5,
  STATE_POCSAG    = 6,
  STATE_M17       = 7,
  STATE_FM        = 10,
  STATE_AX25      = 11,
  STATE_NXDNCAL1K = 91,
  STATE_DMRDMO1K  = 92,
  STATE_P25CAL1K  = 93,
  STATE_DMRCAL1K  = 94,
  STATE_LFCAL     = 95,
  STATE_RSSICAL   = 96,
  STATE_DMRCAL    = 98,
  STATE_DSTARCAL  = 99,
  STATE_INTCAL    = 100,
  STATE_POCSAGCAL = 101,
  STATE_FMCAL10K  = 102,
  STATE_FMCAL12K  = 103,
  STATE_FMCAL15K  = 104,
  STATE_FMCAL20K  = 105,
  STATE_FMCAL25K  = 106,
  STATE_FMCAL30K  = 107,
  STATE_M17CAL    = 108
};

class CMMDVMCal {
public:
	CMMDVMCal(const std::string& port, SERIAL_SPEED speed);
	~CMMDVMCal();

	int run();

private:
	ConfigFile		  m_configFile;				// For configuration file writing
	CEEPROMData*	  m_eepromData;				// For EEPROM writing
	CSerialController m_serial;
	CConsole          m_console;
	CBERCal           m_ber;
	bool              m_transmit;
	bool              m_carrier;
	float             m_txLevel;
	float             m_rxLevel;
	int               m_txDCOffset;
	int               m_rxDCOffset;
	bool              m_txInvert;
	bool              m_rxInvert;
	bool              m_pttInvert;
	unsigned int      m_frequency;
	unsigned int      m_startfrequency;
	unsigned int      m_step;
	float             m_power;
	MMDVM_STATE       m_mode;
	bool              m_duplex;
	bool              m_debug;
	unsigned char*    m_buffer;
	unsigned int      m_length;
	unsigned int      m_offset;
	HW_TYPE           m_hwType;
	unsigned char     m_version;
	bool              m_dstarEnabled;
	bool              m_dmrEnabled;
	bool              m_dmrBERFEC;
	bool              m_ysfEnabled;
	bool              m_p25Enabled;
	bool              m_nxdnEnabled;
	bool              m_m17Enabled;
	bool              m_pocsagEnabled;
	bool              m_fmEnabled;
	bool              m_ax25Enabled;
	// Below added for Nextion Screen Writing
	const char		  m_freqText[10];
	const char		  m_freqOffsetText[10];
	const char		  m_statusText[10];
	// Below added for DMR BER Sweep Test
	float		  	  m_tmpBER;						// Instantaneous BER during freq sweep test
	float		  	  m_tmpBERTotal;				// Accumulated BER during freq sweep test
	bool		  	  m_freqSweep; 					// Are we performing the automated DMR BER test?
	int 		  	  m_tmpBERNum;					// Number of BER readings (used to find average BER for given time period)
	int	  			  m_tmpBERFreqOffsetMin;		// Minimum frequency with detectable BER
	int	  			  m_tmpBERFreqOffsetMax;		// Maximum frequency with detectable BER
	int				  m_tmpBERFreqDir;				// Direction of frequency sweep (-1 = decreasing, 0 = static, 1 = increasing)
	int				  m_tmpBERFreqOffset;			// Current offset from center frequency
	int				  m_tmpBERFreqOffsetFirst;		// The first offset that transmission was detected on (should be 0, but not guaranteed)
	unsigned int	  m_freqSweepCounter;
	const int		  m_freqSweepMin;				// Minimum frequency below center that frequency sweep will go
	const int		  m_freqSweepMax;				// Maximum frequency above center that frequency sweep will go
	int				  m_freqSweepTestResultLast;	// The last test result for optimal DMR Rx offset
	bool			  m_freqSweepTestTaken;			// If no sweep test has been done, we don't have valid data to write to EEPROM

	void displayHelp_MMDVM();
	void displayHelp_MMDVM_HS();
	void loop_MMDVM();
	void loop_MMDVM_HS();
	bool setTransmit();
	bool setTXLevel(int incr);
	bool setRXLevel(int incr);
	bool setTXDCOffset(int incr);
	bool setRXDCOffset(int incr);
	bool setTXInvert();
	bool setRXInvert();
	bool setPTTInvert();
	bool setDebug();
	bool setFreq(int incr);
	bool setStepFreq();
	bool setPower(int incr);
	bool setCarrier();
	bool setEnterFreq();
	bool setFMDeviation();
	bool setDMRDeviation();
	bool setLowFrequencyCal();
	bool setDMRCal1K();
	bool setDMRDMO1K();
	bool setP25Cal1K();
	bool setNXDNCal1K();
	bool setPOCSAGCal();
	bool setDSTARBER_FEC();
	bool setDMRBER_FEC();
	bool setDMRBER_1K();
	bool setYSFBER_FEC();
	bool setP25BER_FEC();
	bool setNXDNBER_FEC();
	bool setDSTAR();
	bool setRSSI();
	bool setM17Cal();
	bool setIntCal();
	bool setFreqSweep(); 										// This is for an automated DMR BER test to find optimal offset
	bool setFreqValue(unsigned int freq, bool changeStart);		// Alternative to setEnterFreq() that does not require user input, option to change m_startfrequency
	void doFreqSweep();											// Does the actual work for setFreqSweep()

	bool EEPROMDisplay();										// Print EEPROM data in human-readable form
	bool writeEEPROMFreqSweepSimplex();							// Write the last freq sweep test result into EEPROM offset data
	bool EEPROMInitialize();									// Write default values into EEPROM
	bool writeEEPROMTxOffset();									// Write current offset
	bool writeEEPROMTxOffset(int offset);						// Write specified offset
	bool writeEEPROMRxOffset();									// Write current offset
	bool writeEEPROMRxOffset(int offset);						// Write specified offset
	bool setNextionText(const char *item, char *txt);			// Set Nextion text field to specified string
	bool setNextionInt(const char *item, int n);				// Set Nextion text field to specified integer
	bool setNextionFloat(const char *item, double f);			// Set Nextion text field to specified float value
	bool sendNextionCmd(char *cmd);								// Send command to Nextion screen
	bool writeCurrentOffsetConfig();							// Write current frequency offset to config file (Tx and Rx)
	bool writeCurrentTxOffsetConfig();							// Write current frequency offset to config file (Tx only)
	bool writeCurrentRxOffsetConfig();							// Write current frequency offset to config file (Rx only)
	bool writeEEPROMOffsetsConfig();							// Write offsets stored in EEPROM to MMDVMHost config file

	bool initModem();
	void displayModem(const unsigned char* buffer, unsigned int length);
	bool writeConfig1(float txlevel, bool debug);
	bool writeConfig2(float txlevel, bool debug);
	void sleep(unsigned int ms);
	bool setFrequency();
	bool getStatus();

	RESP_TYPE_MMDVM getResponse();
};

#endif
