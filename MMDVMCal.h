/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

class CMMDVMCal {
public:
	CMMDVMCal(const std::string& port);
	~CMMDVMCal();

	int run();

private:
	CSerialController m_serial;
	CConsole          m_console;
	bool              m_transmit;
	float             m_txLevel;
	float             m_rxLevel;
	int               m_txDCOffset;
	int               m_rxDCOffset;
	bool              m_txInvert;
	bool              m_rxInvert;
	bool              m_pttInvert;
	unsigned char	  m_mode;

	void displayHelp();
	bool setTransmit();
	bool setTXLevel(int incr);
	bool setRXLevel(int incr);
	bool setTXDCOffset(int incr);
	bool setRXDCOffset(int incr);
	bool setTXInvert();
	bool setRXInvert();
	bool setPTTInvert();
	bool setDMRDeviation();
	bool setLowFrequencyCal();
	bool setDSTAR();
	bool setRSSI();

	bool initModem();
	int  readModem(unsigned char* buffer, unsigned int length);
	void displayModem(const unsigned char* buffer, unsigned int length);
	bool writeConfig();
	void sleep(unsigned int ms);
};

#endif
