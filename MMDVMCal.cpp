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

#include "MMDVMCal.h"

#include <cstdio>
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#define EOL	"\n"
#else
#include <unistd.h>
#define	EOL	"\r\n"
#endif

const unsigned char MMDVM_FRAME_START = 0xE0U;
const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;
const unsigned char MMDVM_CAL_DATA    = 0x08U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned int MAX_RESPONSES = 30U;
const unsigned int BUFFER_LENGTH = 2000U;

#include "Utils.h"

int main(int argc, char** argv)
{
	if (argc < 2) {
		::fprintf(stderr, "Usage: MMDVMCal <port>\n");
		return 1;
	}

	CMMDVMCal cal(argv[1]);

	return cal.run();
}

CMMDVMCal::CMMDVMCal(const std::string& port) :
m_serial(port, SERIAL_115200),
m_console(),
m_transmit(false),
m_txLevel(50.0F),
m_rxLevel(50.0F),
m_txDCOffset(0),
m_rxDCOffset(0),
m_txInvert(false),
m_rxInvert(false),
m_pttInvert(false),
m_mode(STATE_DSTARCAL),
m_buffer(NULL),
m_length(0U),
m_offset(0U)
{
	m_buffer = new unsigned char[BUFFER_LENGTH];
}

CMMDVMCal::~CMMDVMCal()
{
}

int CMMDVMCal::run()
{
	bool ret = m_serial.open();
	if (!ret)
		return 1;

	ret = initModem();
	if (!ret) {
		m_serial.close();
		return 1;
	}

	ret = m_console.open();
	if (!ret) {
		m_serial.close();
		return 1;
	}

	displayHelp();

	bool end = false;
	while (!end) {
		int c = m_console.getChar();
		switch (c) {
			case 'H':
			case 'h':
				displayHelp();
				break;
			case 'T':
				setTXLevel(1);
				break;
			case 't':
				setTXLevel(-1);
				break;
			case 'R':
				setRXLevel(1);
				break;
			case 'r':
				setRXLevel(-1);
				break;
			case ' ':
				setTransmit();
				break;
			case 'I':
				setTXInvert();
				break;
			case 'i':
				setRXInvert();
				break;
			case 'P':
			case 'p':
				setPTTInvert();
				break;
			case 'o':
				setTXDCOffset(-1);
				break;
			case 'O':
				setTXDCOffset(1);
				break;
			case 'c':
				setRXDCOffset(-1);
				break;
			case 'C':
				setRXDCOffset(1);
				break;
			case 'Q':
			case 'q':
				end = true;
				break;
			case 'V':
			case 'v':
				::fprintf(stdout, "MMDVMCal 20170821" EOL);
				break;
			case 'D':
				setDMRDeviation();
				break;
			case 'd':
				setDSTAR();
				break;
			case 'L':
			case 'l':
				setLowFrequencyCal();
				break;
			case 'A':
				setDMRCal1K();
				break;
			case 'M':
			case 'm':
				setDMRDMO1K();
				break;
			case 'a':
				setP25Cal1K();
				break;
			case 'N':
			case 'n':
				setNXDNCal1K();
				break;
			case 'S':
			case 's':
				setRSSI();
				break;
			case -1:
				break;
			default:
				::fprintf(stderr, "Unknown command - %c (H/h for help)" EOL, c);
				break;
		}

		RESP_TYPE_MMDVM resp = getResponse();

		if (resp == RTM_OK)
			displayModem(m_buffer, m_length);

		sleep(5U);
	}

	if (m_transmit)
		setTransmit();

	m_serial.close();
	m_console.close();

	::fprintf(stdout, "PTT Invert: %s, RX Invert: %s, TX Invert: %s, RX Level: %.1f%%, TX Level: %.1f%%, TX DC Offset: %d, RX DC Offset: %d" EOL,
		m_pttInvert ? "yes" : "no", m_rxInvert ? "yes" : "no", m_txInvert ? "yes" : "no",
		m_rxLevel, m_txLevel, m_txDCOffset, m_rxDCOffset);

	return 0;
}

void CMMDVMCal::displayHelp()
{
	::fprintf(stdout, "The commands are:" EOL);
	::fprintf(stdout, "    H/h      Display help" EOL);
	::fprintf(stdout, "    I        Toggle transmit inversion" EOL);
	::fprintf(stdout, "    i        Toggle receive inversion" EOL);
	::fprintf(stdout, "    O        Increase TX DC offset level" EOL);
	::fprintf(stdout, "    o        Decrease TX DC offset level" EOL);
	::fprintf(stdout, "    C        Increase RX DC offset level" EOL);
	::fprintf(stdout, "    c        Decrease RX DC offset level" EOL);
	::fprintf(stdout, "    P/p      Toggle PTT inversion" EOL);
	::fprintf(stdout, "    Q/q      Quit" EOL);
	::fprintf(stdout, "    R        Increase receive level" EOL);
	::fprintf(stdout, "    r        Decrease receive level" EOL);
	::fprintf(stdout, "    T        Increase transmit level" EOL);
	::fprintf(stdout, "    t        Decrease transmit level" EOL);
	::fprintf(stdout, "    D        DMR Deviation Mode (Adjust for 2.75Khz Deviation)" EOL);
	::fprintf(stdout, "    L/l      DMR Low Frequency Mode (80 Hz square wave)" EOL);
	::fprintf(stdout, "    A        DMR BS 1031 Hz Test Pattern (TS2 CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    M/m      DMR MS 1031 Hz Test Pattern (CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    a        P25 1011 Hz Test Pattern (NAC293 ID1 TG1)" EOL);
	::fprintf(stdout, "    N/n      NXDN 1031 Hz Test Pattern (RAN1 ID1 TG1)" EOL);
	::fprintf(stdout, "    d        D-Star Mode" EOL);
	::fprintf(stdout, "    S/s      RSSI Mode" EOL);
	::fprintf(stdout, "    V/v      Display version of MMDVMCal" EOL);
	::fprintf(stdout, "    <space>  Toggle transmit" EOL);
}

bool CMMDVMCal::initModem()
{
	sleep(2000U);	// 2s

	for (unsigned int i = 0U; i < 6U; i++) {
		unsigned char buffer[3U];

		buffer[0U] = MMDVM_FRAME_START;
		buffer[1U] = 3U;
		buffer[2U] = MMDVM_GET_VERSION;

		int ret = m_serial.write(buffer, 3U);
		if (ret != 3)
			return false;

#if defined(__APPLE__)
		m_serial.setNonblock(true);
#endif

		for (unsigned int count = 0U; count < MAX_RESPONSES; count++) {
			sleep(10U);
			RESP_TYPE_MMDVM resp = getResponse();
			if (resp == RTM_OK && m_buffer[2U] == MMDVM_GET_VERSION) {
				::fprintf(stderr, "Version: %u, description: %.*s" EOL, m_buffer[3U], m_length - 4U, m_buffer + 4U);
				if (::memcmp(m_buffer + 4U, "MMDVM ", 6U) == 0)
					m_hwType = HWT_MMDVM;
				else if ((::memcmp(m_buffer + 4U, "ZUMspot", 7U) == 0) || (::memcmp(m_buffer + 4U, "MMDVM_HS_Hat", 12U) == 0) || (::memcmp(m_buffer + 4U, "Nano_hotSPOT", 12U) == 0) || (::memcmp(m_buffer + 4U, "MMDVM_HS-", 9U) == 0))
					m_hwType = HWT_MMDVM_HS;
				else {
					::fprintf(stderr, "Board not supported" EOL);
					return false;
				}

				return writeConfig();

				return true;
			}
		}

		sleep(1500U);
	}

	::fprintf(stderr, "Unable to read the firmware version after six attempts" EOL);

	return false;
}

bool CMMDVMCal::writeConfig()
{
	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 19U;
	buffer[2U] = MMDVM_SET_CONFIG;
	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;
	buffer[4U] = 0x00U;
	buffer[5U] = 0U;
	buffer[6U] = m_mode;
	buffer[7U] = (unsigned char)(m_rxLevel * 2.55F + 0.5F);
	buffer[8U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);
	buffer[9U] = 0U;
	buffer[10U] = 0U;
	buffer[11U] = 128U;
	buffer[12U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);
	buffer[13U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);
	buffer[14U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);
	buffer[15U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);
	buffer[16U] = (unsigned char)(m_txDCOffset + 128);
	buffer[17U] = (unsigned char)(m_rxDCOffset + 128);
	buffer[18U] = (unsigned char)(m_txLevel * 2.55F + 0.5F);

	int ret = m_serial.write(buffer, 19U);
	if (ret <= 0)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				::fprintf(stdout, "The MMDVM is not responding to the SET_CONFIG command" EOL);
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		::fprintf(stdout, "Received a NAK to the SET_CONFIG command from the modem: %u" EOL, m_buffer[4U]);
		return false;
	}

	return true;
}

bool CMMDVMCal::setRXInvert()
{
	m_rxInvert = !m_rxInvert;

	::fprintf(stdout, "RX Invert: %s" EOL, m_rxInvert ? "On" : "Off");

	return writeConfig();
}

bool CMMDVMCal::setTXInvert()
{
	m_txInvert = !m_txInvert;

	::fprintf(stdout, "TX Invert: %s" EOL, m_txInvert ? "On" : "Off");

	return writeConfig();
}

bool CMMDVMCal::setPTTInvert()
{
	m_pttInvert = !m_pttInvert;

	::fprintf(stdout, "PTT Invert: %s" EOL, m_pttInvert ? "On" : "Off");

	return writeConfig();
}

bool CMMDVMCal::setDMRDeviation()
{
	m_mode = STATE_DMRCAL;

	::fprintf(stdout, "DMR Deviation Mode (Set to 2.75Khz Deviation)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setLowFrequencyCal()
{
	m_mode = STATE_LFCAL;

	::fprintf(stdout, "DMR Low Frequency Mode (80 Hz square wave)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setDMRCal1K()
{
	m_mode = STATE_DMRCAL1K;

	::fprintf(stdout, "DMR BS 1031 Hz Test Pattern (TS2 CC1 ID1 TG9)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setDMRDMO1K()
{
	m_mode = STATE_DMRDMO1K;

	::fprintf(stdout, "DMR MS 1031 Hz Test Pattern (CC1 ID1 TG9)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setP25Cal1K()
{
	m_mode = STATE_P25CAL1K;

	::fprintf(stdout, "P25 1011 Hz Test Pattern (NAC293 ID1 TG1)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setNXDNCal1K()
{
	m_mode = STATE_NXDNCAL1K;

	::fprintf(stdout, "NXDN 1031 Hz Test Pattern (RAN1 ID1 TG1)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setDSTAR()
{
	m_mode = STATE_DSTARCAL;

	::fprintf(stdout, "D-Star Mode" EOL);

	return writeConfig();
}

bool CMMDVMCal::setRSSI()
{
	m_mode = STATE_RSSICAL;

	::fprintf(stdout, "RSSI Mode" EOL);

	return writeConfig();
}

bool CMMDVMCal::setRXLevel(int incr)
{
	if (incr > 0 && m_rxLevel < 100.0F) {
		m_rxLevel += 0.5F;
		::fprintf(stdout, "RX Level: %.1f%%" EOL, m_rxLevel);
		return writeConfig();
	}

	if (incr < 0 && m_rxLevel > 0.0F) {
		m_rxLevel -= 0.5F;
		::fprintf(stdout, "RX Level: %.1f%%" EOL, m_rxLevel);
		return writeConfig();
	}

	return true;
}

bool CMMDVMCal::setTXLevel(int incr)
{
	if (incr > 0 && m_txLevel < 100.0F) {
		m_txLevel += 0.5F;
		::fprintf(stdout, "TX Level: %.1f%%" EOL, m_txLevel);
		return writeConfig();
	}

	if (incr < 0 && m_txLevel > 0.0F) {
		m_txLevel -= 0.5F;
		::fprintf(stdout, "TX Level: %.1f%%" EOL, m_txLevel);
		return writeConfig();
	}

	return true;
}

bool CMMDVMCal::setTXDCOffset(int incr)
{
	if (incr > 0 && m_txDCOffset < 127) {
		m_txDCOffset++;
		::fprintf(stdout, "TX DC Offset: %d" EOL, m_txDCOffset);
		return writeConfig();
	}

	if (incr < 0 && m_txDCOffset > -128) {
		m_txDCOffset--;
		::fprintf(stdout, "TX DC Offset: %d" EOL, m_txDCOffset);
		return writeConfig();
	}

	return true;
}

bool CMMDVMCal::setRXDCOffset(int incr)
{
	if (incr > 0 && m_rxDCOffset < 127) {
		m_rxDCOffset++;
		::fprintf(stdout, "RX DC Offset: %d" EOL, m_rxDCOffset);
		return writeConfig();
	}

	if (incr < 0 && m_rxDCOffset > -128) {
		m_rxDCOffset--;
		::fprintf(stdout, "RX DC Offset: %d" EOL, m_rxDCOffset);
		return writeConfig();
	}

	return true;
}

bool CMMDVMCal::setTransmit()
{
	m_transmit = !m_transmit;

	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 4U;
	buffer[2U] = MMDVM_CAL_DATA;
	buffer[3U] = m_transmit ? 0x01U : 0x00U;

	int ret = m_serial.write(buffer, 4U);
	if (ret <= 0)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				::fprintf(stdout, "The MMDVM is not responding to the CAL_DATA command" EOL);
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		::fprintf(stdout, "Received a NAK to the CAL_DATA command from the modem: %u" EOL, m_buffer[4U]);
		return false;
	}

	return true;
}

void CMMDVMCal::displayModem(const unsigned char *buffer, unsigned int length)
{
	if (buffer[2U] == 0x08U) {
		bool  inverted = (buffer[3U] == 0x80U);
		short high = buffer[4U] << 8 | buffer[5U];
		short low = buffer[6U] << 8 | buffer[7U];
		short diff = high - low;
		short centre = (high + low) / 2;
		::fprintf(stdout, "Levels: inverted: %s, max: %d, min: %d, diff: %d, centre: %d" EOL, inverted ? "yes" : "no", high, low, diff, centre);
	} else if (buffer[2U] == 0x09U) {
		unsigned short max = buffer[3U] << 8 | buffer[4U];
		unsigned short min = buffer[5U] << 8 | buffer[6U];
		unsigned short ave = buffer[7U] << 8 | buffer[8U];
		::fprintf(stdout, "RSSI: max: %u, min: %u, ave: %u" EOL, max, min, ave);
	} else if (buffer[2U] == 0xF1U) {
		::fprintf(stdout, "Debug: %.*s" EOL, length - 3U, buffer + 3U);
	} else if (buffer[2U] == 0xF2U) {
		short val1 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d" EOL, length - 5U, buffer + 3U, val1);
	} else if (buffer[2U] == 0xF3U) {
		short val1 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val2 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d" EOL, length - 7U, buffer + 3U, val1, val2);
	} else if (buffer[2U] == 0xF4U) {
		short val1 = (buffer[length - 6U] << 8) | buffer[length - 5U];
		short val2 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val3 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d %d" EOL, length - 9U, buffer + 3U, val1, val2, val3);
	} else if (buffer[2U] == 0xF5U) {
		short val1 = (buffer[length - 8U] << 8) | buffer[length - 7U];
		short val2 = (buffer[length - 6U] << 8) | buffer[length - 5U];
		short val3 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val4 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d %d %d" EOL, length - 11U, buffer + 3U, val1, val2, val3, val4);
	} else {
		CUtils::dump("Response", buffer, length);
	}
}

RESP_TYPE_MMDVM CMMDVMCal::getResponse()
{
	if (m_offset == 0U) {
		// Get the start of the frame or nothing at all
		int ret = m_serial.read(m_buffer + 0U, 1U);
		if (ret < 0) {
			::fprintf(stderr, "Error when reading from the modem" EOL);
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[0U] != MMDVM_FRAME_START)
			return RTM_TIMEOUT;

		m_offset = 1U;
	}

	if (m_offset == 1U) {
		// Get the length of the frame
		int ret = m_serial.read(m_buffer + 1U, 1U);
		if (ret < 0) {
			::fprintf(stderr, "Error when reading from the modem" EOL);
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		if (m_buffer[1U] >= 250U) {
			::fprintf(stderr, "Invalid length received from the modem - %u" EOL, m_buffer[1U]);
			m_offset = 0U;
			return RTM_ERROR;
		}

		m_length = m_buffer[1U];
		m_offset = 2U;
	}

	if (m_offset == 2U) {
		// Get the frame type
		int ret = m_serial.read(m_buffer + 2U, 1U);
		if (ret < 0) {
			::fprintf(stderr, "Error when reading from the modem" EOL);
			m_offset = 0U;
			return RTM_ERROR;
		}

		if (ret == 0)
			return RTM_TIMEOUT;

		m_offset = 3U;
	}

	if (m_offset >= 3U) {
		// Use later two byte length field
		if (m_length == 0U) {
			int ret = m_serial.read(m_buffer + 3U, 2U);
			if (ret < 0) {
				::fprintf(stderr, "Error when reading from the modem" EOL);
				m_offset = 0U;
				return RTM_ERROR;
			}

			if (ret == 0)
				return RTM_TIMEOUT;

			m_length = (m_buffer[3U] << 8) | m_buffer[4U];
			m_offset = 5U;
		}

		while (m_offset < m_length) {
			int ret = m_serial.read(m_buffer + m_offset, m_length - m_offset);
			if (ret < 0) {
				::fprintf(stderr, "Error when reading from the modem" EOL);
				m_offset = 0U;
				return RTM_ERROR;
			}

			if (ret == 0)
				return RTM_TIMEOUT;

			if (ret > 0)
				m_offset += ret;
		}
	}

	m_offset = 0U;

	return RTM_OK;
}

void CMMDVMCal::sleep(unsigned int ms)
{
#if defined(_WIN32) || defined(_WIN64)
	::Sleep(ms);
#else
	::usleep(ms * 1000);
#endif
}
