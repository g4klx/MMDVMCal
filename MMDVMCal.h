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

#include <cstring>
#include <cstdlib>

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
