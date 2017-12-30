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

#if defined(_WIN32) || defined(_WIN64)
#define EOL	"\n"
#else
#include <unistd.h>
#define	EOL	"\r\n"
#endif

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
m_mode(99U)
{
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

		unsigned char buffer[130U];
		int n = readModem(buffer, 130U);
		if (n > 0)
			displayModem(buffer, n);

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
	::fprintf(stdout, "    d        D-Star Mode" EOL);
	::fprintf(stdout, "    S/s      RSSI Mode" EOL);
	::fprintf(stdout, "    V/v      Display version of MMDVMCal" EOL);
	::fprintf(stdout, "    <space>  Toggle transmit" EOL);
}

bool CMMDVMCal::initModem()
{
	unsigned char buffer[150U];

	sleep(2000U);

	int ret = 0;
	for (unsigned int i = 0U; i < 5U && ret <= 0; i++) {
		buffer[0U] = 0xE0U;
		buffer[1U] = 3U;
		buffer[2U] = 0x00U;

		ret = m_serial.write(buffer, 3U);
		if (ret <= 0)
			return false;

		sleep(100U);

		ret = readModem(buffer, 50U);
		if (ret < 0)
			return false;
		if (ret == 0)
			sleep(1000U);
	}

	if (ret <= 0) {
		::fprintf(stderr, "No response from the modem" EOL);
		return false;
	}

	if (buffer[2U] != 0x00U) {
		CUtils::dump("Invalid response", buffer, ret);
		return false;
	}

	::fprintf(stdout, "Version: %u \"%.*s\"" EOL, buffer[3U], buffer[1U] - 4, buffer + 4U);

	return writeConfig();
}

bool CMMDVMCal::writeConfig()
{
	unsigned char buffer[50U];

	buffer[0U] = 0xE0U;
	buffer[1U] = 18U;
	buffer[2U] = 0x02U;
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

	int ret = m_serial.write(buffer, 18U);
	if (ret <= 0)
		return false;

	sleep(10U);

	ret = readModem(buffer, 50U);
	if (ret <= 0)
		return false;

	if (buffer[2U] == 0x7FU) {
		::fprintf(stderr, "Got a NAK from the modem" EOL);
		return false;
	}

	if (buffer[2U] != 0x70U) {
		CUtils::dump("Invalid response", buffer, ret);
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
	m_mode=98;

	::fprintf(stdout, "DMR Deviation Mode (Set to 2.75Khz Deviation)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setLowFrequencyCal()
{
	m_mode=95;

	::fprintf(stdout, "DMR Low Frequency Mode (80 Hz square wave)" EOL);

	return writeConfig();
}

bool CMMDVMCal::setDSTAR()
{
	m_mode= 99;

	::fprintf(stdout, "D-Star Mode" EOL);

	return writeConfig();
}

bool CMMDVMCal::setRSSI()
{
	m_mode = 96;

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

	buffer[0U] = 0xE0U;
	buffer[1U] = 4U;
	buffer[2U] = 0x08U;
	buffer[3U] = m_transmit ? 0x01U : 0x00U;

	int ret = m_serial.write(buffer, 4U);
	if (ret <= 0)
		return false;

	sleep(10U);

	ret = readModem(buffer, 50U);
	if (ret <= 0)
		return false;

	if (buffer[2U] == 0x7FU) {
		::fprintf(stderr, "Got a NAK from the modem" EOL);
		return false;
	}

	if (buffer[2U] != 0x70U) {
		CUtils::dump("Invalid response", buffer, ret);
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

int CMMDVMCal::readModem(unsigned char *buffer, unsigned int length)
{
	int n = m_serial.read(buffer + 0U, 1U);
	if (n <= 0)
		return n;

	if (buffer[0U] != 0xE0U)
		return 0;

	n = 0;
	for (unsigned int i = 0U; i < 20U && n == 0; i++) {
		n = m_serial.read(buffer + 1U, 1U);
		if (n < 0)
			return n;
		if (n == 0)
			sleep(10U);
	}

	if (n == 0)
		return -1;

	unsigned int len = buffer[1U];

	unsigned int offset = 2U;
	for (unsigned int i = 0U; i < 20U && offset < len; i++) {
		n = m_serial.read(buffer + offset, len - offset);
		if (n < 0)
			return n;
		if (n == 0)
			sleep(10U);
		if (n > 0)
			offset += n;
	}

	return len;
}

void CMMDVMCal::sleep(unsigned int ms)
{
#if defined(_WIN32) || defined(_WIN64)
	::Sleep(ms);
#else
	::usleep(ms * 1000);
#endif
}
