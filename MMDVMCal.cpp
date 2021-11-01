/*
 *   Copyright (C) 2015,2016,2017,2018,2020,2021 by Jonathan Naylor G4KLX
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

#include "MMDVMCal.h"
#include "Version.h"

#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)
#define EOL	"\n"
#else
#include <unistd.h>
#define	EOL	"\r\n"
#endif

#include "Utils.h"

const unsigned char MMDVM_GET_STATUS  = 0x01U;
const unsigned char MMDVM_FRAME_START = 0xE0U;
const unsigned char MMDVM_GET_VERSION = 0x00U;
const unsigned char MMDVM_SET_CONFIG  = 0x02U;
const unsigned char MMDVM_SET_FREQ    = 0x04U;
const unsigned char MMDVM_CAL_DATA    = 0x08U;
const unsigned char MMDVM_RSSI_DATA   = 0x09U;

const unsigned char MMDVM_DSTAR_HEADER = 0x10U;
const unsigned char MMDVM_DSTAR_DATA   = 0x11U;
const unsigned char MMDVM_DSTAR_LOST   = 0x12U;
const unsigned char MMDVM_DSTAR_EOT    = 0x13U;

const unsigned char MMDVM_DMR_DATA1   = 0x18U;
const unsigned char MMDVM_DMR_LOST1   = 0x19U;
const unsigned char MMDVM_DMR_DATA2   = 0x1AU;
const unsigned char MMDVM_DMR_LOST2   = 0x1BU;
const unsigned char MMDVM_DMR_SHORTLC = 0x1CU;
const unsigned char MMDVM_DMR_START   = 0x1DU;
const unsigned char MMDVM_DMR_ABORT   = 0x1EU;

const unsigned char MMDVM_YSF_DATA    = 0x20U;
const unsigned char MMDVM_YSF_LOST    = 0x21U;

const unsigned char MMDVM_P25_HDR     = 0x30U;
const unsigned char MMDVM_P25_LDU     = 0x31U;
const unsigned char MMDVM_P25_LOST    = 0x32U;

const unsigned char MMDVM_NXDN_DATA   = 0x40U;
const unsigned char MMDVM_NXDN_LOST   = 0x41U;

const unsigned char MMDVM_M17_LINK_SETUP = 0x45U;
const unsigned char MMDVM_M17_STREAM     = 0x46U;
const unsigned char MMDVM_M17_PACKET     = 0x47U;
const unsigned char MMDVM_M17_LOST       = 0x48U;
const unsigned char MMDVM_M17_EOT        = 0x49U;

const unsigned char MMDVM_POCSAG_DATA = 0x50U;

const unsigned char MMDVM_AX25_DATA   = 0x55U;

const unsigned char MMDVM_FM_PARAMS1  = 0x60U;
const unsigned char MMDVM_FM_PARAMS2  = 0x61U;
const unsigned char MMDVM_FM_PARAMS3  = 0x62U;
const unsigned char MMDVM_FM_PARAMS4  = 0x63U;
const unsigned char MMDVM_FM_DATA     = 0x65U;
const unsigned char MMDVM_FM_CONTROL  = 0x66U;
const unsigned char MMDVM_FM_EOT      = 0x67U;

const unsigned char MMDVM_ACK         = 0x70U;
const unsigned char MMDVM_NAK         = 0x7FU;

const unsigned char MMDVM_DEBUG1      = 0xF1U;
const unsigned char MMDVM_DEBUG2      = 0xF2U;
const unsigned char MMDVM_DEBUG3      = 0xF3U;
const unsigned char MMDVM_DEBUG4      = 0xF4U;
const unsigned char MMDVM_DEBUG5      = 0xF5U;

const unsigned int MAX_RESPONSES = 30U;
const unsigned int BUFFER_LENGTH = 2000U;

int main(int argc, char** argv)
{
	if (argc < 3) {
		::fprintf(stderr, "Usage: MMDVMCal <speed> <port>\n");
		return 1;
	}

	SERIAL_SPEED speed = SERIAL_460800;

	if (::strcmp(argv[1], "115200") == 0)
		speed = SERIAL_115200;
	else if (::strcmp(argv[1], "230400") == 0)
		speed = SERIAL_230400;
	else if (::strcmp(argv[1], "460800") == 0)
		speed = SERIAL_460800;
	else
		::fprintf(stderr, "MMDVMCal: unknown speed - %s, using 460800\n", argv[1]);

	CMMDVMCal cal(argv[2], speed);

	return cal.run();
}

CMMDVMCal::CMMDVMCal(const std::string& port, SERIAL_SPEED speed) :
m_serial(port, speed),
m_console(),
m_transmit(false),
m_carrier(false),
m_txLevel(50.0F),
m_rxLevel(50.0F),
m_txDCOffset(0),
m_rxDCOffset(0),
m_txInvert(false),
m_rxInvert(false),
m_pttInvert(false),
m_frequency(433000000U),
m_startfrequency(433000000U),
m_step(50U),
m_power(100.0F),
m_mode(STATE_DSTARCAL),
m_duplex(true),
m_debug(false),
m_buffer(NULL),
m_length(0U),
m_offset(0U),
m_hwType(),
m_version(0U),
m_dstarEnabled(false),
m_dmrEnabled(false),
m_dmrBERFEC(true),
m_ysfEnabled(false),
m_p25Enabled(false),
m_nxdnEnabled(false),
m_m17Enabled(false),
m_pocsagEnabled(false),
m_fmEnabled(false),
m_ax25Enabled(false)
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

	if (m_hwType == HWT_MMDVM)
		loop_MMDVM();
	else if (m_hwType == HWT_MMDVM_HS)
		loop_MMDVM_HS();

	if (m_transmit)
		setTransmit();

	m_serial.close();
	m_console.close();

	if (m_hwType == HWT_MMDVM) {
		::fprintf(stdout, "PTT Invert: %s, RX Invert: %s, TX Invert: %s, RX Level: %.1f%%, TX Level: %.1f%%, TX DC Offset: %d, RX DC Offset: %d" EOL,
			m_pttInvert ? "yes" : "no", m_rxInvert ? "yes" : "no", m_txInvert ? "yes" : "no",
			m_rxLevel, m_txLevel, m_txDCOffset, m_rxDCOffset);
	}
	else if (m_hwType == HWT_MMDVM_HS) {
		::fprintf(stdout, "TX Level: %.1f%%, Frequency Offset: %d, RF Level: %.1f%%" EOL, m_txLevel, (int)(m_frequency - m_startfrequency), m_power);
	}

	return 0;
}

void CMMDVMCal::loop_MMDVM()
{
	displayHelp_MMDVM();

	unsigned int counter = 0U;
	bool end = false;
	while (!end) {
		int c = m_console.getChar();
		switch (c) {
			case 'H':
			case 'h':
				displayHelp_MMDVM();
				break;
			case 'W':
			case 'w':
				setDebug();
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
				::fprintf(stdout, VERSION EOL);
				break;
			case 'F':
				setFMDeviation();
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
			case 'K':
			case 'k':
				setDSTARBER_FEC();
				break;
			case 'b':
				setDMRBER_FEC();
				break;
			case 'B':
				setDMRBER_1K();
				break;
			case 'J':
				setYSFBER_FEC();
				break;
			case 'j':
				setP25BER_FEC();
				break;
			case 'n':
				setNXDNBER_FEC();
				break;
			case 'a':
				setP25Cal1K();
				break;
			case 'N':
				setNXDNCal1K();
				break;
			case 'g':
				setPOCSAGCal();
				break;
			case 'S':
			case 's':
				setRSSI();
				break;
			case 'e':
				setM17Cal();
				break;
			case -1:
			case  0:
				break;
			default:
				::fprintf(stderr, "Unknown command - %c (H/h for help)" EOL, c);
				break;
	    	}

  	    	RESP_TYPE_MMDVM resp = getResponse();

		if (resp == RTM_OK)
			displayModem(m_buffer, m_length);

		m_ber.clock();
		sleep(5U);

		if (counter >= 200U) {
			if (getStatus())
				displayModem(m_buffer, m_length);
			counter = 0U;
		}

		counter++;
	}
}

void CMMDVMCal::displayHelp_MMDVM()
{
	::fprintf(stdout, "The commands are:" EOL);
	::fprintf(stdout, "    H/h      Display help" EOL);
	::fprintf(stdout, "    Q/q      Quit" EOL);
	::fprintf(stdout, "    W/w      Enable/disable modem debug messages" EOL);
	::fprintf(stdout, "    I        Toggle transmit inversion" EOL);
	::fprintf(stdout, "    i        Toggle receive inversion" EOL);
	::fprintf(stdout, "    O        Increase TX DC offset level" EOL);
	::fprintf(stdout, "    o        Decrease TX DC offset level" EOL);
	::fprintf(stdout, "    C        Increase RX DC offset level" EOL);
	::fprintf(stdout, "    c        Decrease RX DC offset level" EOL);
	::fprintf(stdout, "    P/p      Toggle PTT inversion" EOL);
	::fprintf(stdout, "    R        Increase receive level" EOL);
	::fprintf(stdout, "    r        Decrease receive level" EOL);
	::fprintf(stdout, "    T        Increase transmit level" EOL);
	::fprintf(stdout, "    t        Decrease transmit level" EOL);
	::fprintf(stdout, "    d        D-Star Mode" EOL);
	::fprintf(stdout, "    F        FM Deviation Mode (Adjust for correct Deviation)" EOL);
	::fprintf(stdout, "    D        DMR Deviation Mode (Adjust for 2.75Khz Deviation)" EOL);
	::fprintf(stdout, "    L/l      DMR Low Frequency Mode (80 Hz square wave)" EOL);
	::fprintf(stdout, "    A        DMR Duplex 1031 Hz Test Pattern (TS2 CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    M/m      DMR Simplex 1031 Hz Test Pattern (CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    a        P25 1011 Hz Test Pattern (NAC293 ID1 TG1)" EOL);
	::fprintf(stdout, "    N        NXDN 1031 Hz Test Pattern (RAN1 ID1 TG1)" EOL);
	::fprintf(stdout, "    K/k      BER Test Mode (FEC) for D-Star" EOL);
	::fprintf(stdout, "    b        BER Test Mode (FEC) for DMR Simplex (CC1)" EOL);
	::fprintf(stdout, "    B        BER Test Mode (1031 Hz Test Pattern) for DMR Simplex (CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    J        BER Test Mode (FEC) for YSF" EOL);
	::fprintf(stdout, "    j        BER Test Mode (FEC) for P25" EOL);
	::fprintf(stdout, "    n        BER Test Mode (FEC) for NXDN" EOL);
	::fprintf(stdout, "    g        POCSAG 600Hz Test Pattern" EOL);
	::fprintf(stdout, "    e        M17 Preamble Test Pattern" EOL);
	::fprintf(stdout, "    S/s      RSSI Mode" EOL);
	::fprintf(stdout, "    V/v      Display version of MMDVMCal" EOL);
	::fprintf(stdout, "    <space>  Toggle transmit" EOL);
}

void CMMDVMCal::loop_MMDVM_HS()
{
	m_mode = STATE_DMRCAL;
	unsigned int counter = 0U;

	setFrequency();

	switch (m_version) {
	case 1U:
		writeConfig1(m_txLevel, m_debug);
		break;
	case 2U:
		writeConfig2(m_txLevel, m_debug);
		break;
	}

	displayHelp_MMDVM_HS();

	bool end = false;
	while (!end) {
		int c = m_console.getChar();
		switch (c) {
			case 'H':
			case 'h':
				displayHelp_MMDVM_HS();
				break;
			case 'W':
			case 'w':
				setDebug();
				break;
			case 'C':
			case 'c':
				setCarrier();
				break;
			case 'E':
			case 'e':
				setEnterFreq();
				break;
			case 'T':
				setTXLevel(1);
				break;
			case 't':
				setTXLevel(-1);
				break;
			case 'F':
				setFreq(1);
				break;
			case 'f':
				setFreq(-1);
				break;
			case 'Z':
			case 'z':
				setStepFreq();
				break;
			case 'P':
				setPower(1);
				break;
			case 'p':
				setPower(-1);
				break;
			case ' ':
				setTransmit();
				break;
			case 'Q':
			case 'q':
				end = true;
				break;
			case 'V':
			case 'v':
				::fprintf(stdout, VERSION EOL);
				break;
			case 'D':
			case 'd':
				setDMRDeviation();
				break;
			case 'M':
			case 'm':
				setDMRDMO1K();
				break;
			case 'K':
			case 'k':
				setDSTARBER_FEC();
				break;
			case 'b':
				setDMRBER_FEC();
				break;
			case 'B':
				setDMRBER_1K();
				break;
			case 'J':
				setYSFBER_FEC();
				break;
			case 'j':
				setP25BER_FEC();
				break;
			case 'n':
				setNXDNBER_FEC();
				break;
			case 'g':
				setPOCSAGCal();
				break;
			case 'S':
			case 's':
				setRSSI();
				break;
			case 'I':
			case 'i':
				setIntCal();
				break;
			case -1:
			case  0:
				break;
			default:
				::fprintf(stderr, "Unknown command - %c (H/h for help)" EOL, c);
				break;
		}

		RESP_TYPE_MMDVM resp = getResponse();

		if (resp == RTM_OK)
			displayModem(m_buffer, m_length);

		m_ber.clock();
		sleep(5U);

		if (counter >= 200U) {
			if (getStatus())
	    		displayModem(m_buffer, m_length);
			counter = 0U;
		}

		counter++; 
	}
}

void CMMDVMCal::displayHelp_MMDVM_HS()
{
	::fprintf(stdout, "The commands are:" EOL);
	::fprintf(stdout, "    H/h      Display help" EOL);
	::fprintf(stdout, "    Q/q      Quit" EOL);
	::fprintf(stdout, "    W/w      Enable/disable modem debug messages" EOL);
	::fprintf(stdout, "    E/e      Enter frequency (current: %u Hz)" EOL, m_frequency);
	::fprintf(stdout, "    F        Increase frequency" EOL);
	::fprintf(stdout, "    f        Decrease frequency" EOL);
	::fprintf(stdout, "    Z/z      Enter frequency step" EOL);
	::fprintf(stdout, "    T        Increase deviation" EOL);
	::fprintf(stdout, "    t        Decrease deviation" EOL);
	::fprintf(stdout, "    P        Increase RF power" EOL);
	::fprintf(stdout, "    p        Decrease RF power" EOL);
	::fprintf(stdout, "    C/c      Carrier Only Mode" EOL);
	::fprintf(stdout, "    D/d      DMR Deviation Mode (Adjust for 2.75Khz Deviation)" EOL);
	::fprintf(stdout, "    M/m      DMR Simplex 1031 Hz Test Pattern (CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    K/k      BER Test Mode (FEC) for D-Star" EOL);
	::fprintf(stdout, "    b        BER Test Mode (FEC) for DMR Simplex (CC1)" EOL);
	::fprintf(stdout, "    B        BER Test Mode (1031 Hz Test Pattern) for DMR Simplex (CC1 ID1 TG9)" EOL);
	::fprintf(stdout, "    J        BER Test Mode (FEC) for YSF" EOL);
	::fprintf(stdout, "    j        BER Test Mode (FEC) for P25" EOL);
	::fprintf(stdout, "    n        BER Test Mode (FEC) for NXDN" EOL);
	::fprintf(stdout, "    g        POCSAG 600Hz Test Pattern" EOL);
	::fprintf(stdout, "    S/s      RSSI Mode" EOL);
	::fprintf(stdout, "    I/i      Interrupt Counter Mode" EOL);
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
				m_version = m_buffer[3U];

				switch (m_version) {
				case 1U:
					::fprintf(stderr, "Version: 1, description: %.*s" EOL, m_length - 4U, m_buffer + 4U);
					if (::memcmp(m_buffer + 4U, "MMDVM ", 6U) == 0)
						m_hwType = HWT_MMDVM;
					else if ((::memcmp(m_buffer + 4U, "ZUMspot", 7U) == 0) || (::memcmp(m_buffer + 4U, "MMDVM_HS_Hat", 12U) == 0) || (::memcmp(m_buffer + 4U, "MMDVM_HS_Dual_Hat", 17U) == 0) || (::memcmp(m_buffer + 4U, "Nano_hotSPOT", 12U) == 0) || (::memcmp(m_buffer + 4U, "Nano_DV", 7U) == 0) || (::memcmp(m_buffer + 4U, "MMDVM_HS-", 9U) == 0) || (::memcmp(m_buffer + 4U, "D2RG_MMDVM_HS", 13U) == 0) || (::memcmp(m_buffer + 4U, "SkyBridge", 9U) == 0))
						m_hwType = HWT_MMDVM_HS;
					else {
						::fprintf(stderr, "Board not supported" EOL);
						return false;
					}
					return writeConfig1(m_txLevel, m_debug);

				case 2U:
					::fprintf(stderr, "Version: 2, description: %.*s" EOL, m_length - 23U, m_buffer + 23U);
					if (::memcmp(m_buffer + 23U, "MMDVM ", 6U) == 0)
						m_hwType = HWT_MMDVM;
					else if ((::memcmp(m_buffer + 23U, "ZUMspot", 7U) == 0) || (::memcmp(m_buffer + 23U, "MMDVM_HS_Hat", 12U) == 0) || (::memcmp(m_buffer + 23U, "MMDVM_HS_Dual_Hat", 17U) == 0) || (::memcmp(m_buffer + 23U, "Nano_hotSPOT", 12U) == 0) || (::memcmp(m_buffer + 23U, "Nano_DV", 7U) == 0) || (::memcmp(m_buffer + 23U, "MMDVM_HS-", 9U) == 0) || (::memcmp(m_buffer + 23U, "D2RG_MMDVM_HS", 13U) == 0) || (::memcmp(m_buffer + 23U, "SkyBridge", 9U) == 0))
						m_hwType = HWT_MMDVM_HS;
					else {
						::fprintf(stderr, "Board not supported" EOL);
						return false;
					}
					switch (m_buffer[6U]) {
					case 0U:
						::fprintf(stderr, "CPU: Atmel ARM, UDID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", m_buffer[7U], m_buffer[8U], m_buffer[9U], m_buffer[10U], m_buffer[11U], m_buffer[12U], m_buffer[13U], m_buffer[14U], m_buffer[15U], m_buffer[16U], m_buffer[17U], m_buffer[18U], m_buffer[19U], m_buffer[20U], m_buffer[21U], m_buffer[22U]);
						break;
					case 1U:
						::fprintf(stderr, "CPU: NXP ARM, UDID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", m_buffer[7U], m_buffer[8U], m_buffer[9U], m_buffer[10U], m_buffer[11U], m_buffer[12U], m_buffer[13U], m_buffer[14U], m_buffer[15U], m_buffer[16U], m_buffer[17U], m_buffer[18U], m_buffer[19U], m_buffer[20U], m_buffer[21U], m_buffer[22U]);
						break;
					case 2U:
						::fprintf(stderr, "CPU: ST-Micro ARM, UDID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", m_buffer[7U], m_buffer[8U], m_buffer[9U], m_buffer[10U], m_buffer[11U], m_buffer[12U], m_buffer[13U], m_buffer[14U], m_buffer[15U], m_buffer[16U], m_buffer[17U], m_buffer[18U]);
						break;
					default:
						::fprintf(stderr, "CPU: Unknown type: %u", m_buffer[6U]);
						break;
					}
					::fprintf(stderr, "Modes:");
					if ((m_buffer[4U] & 0x01U) == 0x01U)
						::fprintf(stderr, " D-Star");
					if ((m_buffer[4U] & 0x02U) == 0x02U)
						::fprintf(stderr, " DMR");
					if ((m_buffer[4U] & 0x04U) == 0x04U)
						::fprintf(stderr, " YSF");
					if ((m_buffer[4U] & 0x08U) == 0x08U)
						::fprintf(stderr, " P25");
					if ((m_buffer[4U] & 0x10U) == 0x10U)
						::fprintf(stderr, " NXDN");
					if ((m_buffer[4U] & 0x20U) == 0x20U)
						::fprintf(stderr, " M17");
					if ((m_buffer[4U] & 0x40U) == 0x40U)
						::fprintf(stderr, " FM");
					if ((m_buffer[5U] & 0x01U) == 0x01U)
						::fprintf(stderr, " POCSAG");
					if ((m_buffer[5U] & 0x02U) == 0x02U)
						::fprintf(stderr, " AX.25");
					::fprintf(stderr, EOL);
					return writeConfig2(m_txLevel, m_debug);

				default:
					::fprintf(stderr, "Version not supported" EOL);
					return false;
				}

				return true;
			}
		}

		sleep(1500U);
	}

	::fprintf(stderr, "Unable to read the firmware version after six attempts" EOL);

	return false;
}

bool CMMDVMCal::writeConfig1(float txlevel, bool debug)
{
	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 27U;
	buffer[2U] = MMDVM_SET_CONFIG;

	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;
	if (!m_duplex)
		buffer[3U] |= 0x80U;
	if (debug)
		buffer[3U] |= 0x10U;

	buffer[4U] = 0x00U;
	if (m_dstarEnabled)
		buffer[4U] |= 0x01U;
	if (m_dmrEnabled)
		buffer[4U] |= 0x02U;
	if (m_ysfEnabled)
		buffer[4U] |= 0x04U;
	if (m_p25Enabled)
		buffer[4U] |= 0x08U;
	if (m_nxdnEnabled)
		buffer[4U] |= 0x10U;
	if (m_pocsagEnabled)
		buffer[4U] |= 0x20U;
	if (m_fmEnabled)
		buffer[4U] |= 0x40U;
	if (m_ax25Enabled)
		buffer[4U] |= 0x80U;

	buffer[5U] = 0U;
	buffer[6U] = m_mode;
	buffer[7U] = (unsigned char)(m_rxLevel * 2.55F + 0.5F);
	buffer[8U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[9U] = 1U;
	buffer[10U] = 0U;
	buffer[11U] = 128U;
	buffer[12U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[13U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[14U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[15U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[16U] = (unsigned char)(m_txDCOffset + 128);
	buffer[17U] = (unsigned char)(m_rxDCOffset + 128);
	buffer[18U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[19U] = 0U;
	buffer[20U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[21U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[22U] = 0U;
	buffer[23U] = 0U;
	buffer[24U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[25U] = 134U;		// +6
	buffer[26U] = 0U;

	int ret = m_serial.write(buffer, 27U);
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
bool CMMDVMCal::writeConfig2(float txlevel, bool debug)
{
	unsigned char buffer[50U];

	buffer[0U] = MMDVM_FRAME_START;
	buffer[1U] = 40U;
	buffer[2U] = MMDVM_SET_CONFIG;

	buffer[3U] = 0x00U;
	if (m_rxInvert)
		buffer[3U] |= 0x01U;
	if (m_txInvert)
		buffer[3U] |= 0x02U;
	if (m_pttInvert)
		buffer[3U] |= 0x04U;
	if (!m_duplex)
		buffer[3U] |= 0x80U;
	if (debug)
		buffer[3U] |= 0x10U;

	buffer[4U] = 0x00U;
	if (m_dstarEnabled)
		buffer[4U] |= 0x01U;
	if (m_dmrEnabled)
		buffer[4U] |= 0x02U;
	if (m_ysfEnabled)
		buffer[4U] |= 0x04U;
	if (m_p25Enabled)
		buffer[4U] |= 0x08U;
	if (m_nxdnEnabled)
		buffer[4U] |= 0x10U;
	if (m_fmEnabled)
		buffer[4U] |= 0x20U;
	if (m_m17Enabled)
		buffer[4U] |= 0x40U;

	buffer[5U] = 0x00U;
	if (m_pocsagEnabled)
		buffer[5U] |= 0x01U;
	if (m_ax25Enabled)
		buffer[5U] |= 0x02U;

	buffer[6U] = 0U;

	buffer[7U] = m_mode;

	buffer[8U] = (unsigned char)(m_txDCOffset + 128);
	buffer[9U] = (unsigned char)(m_rxDCOffset + 128);

	buffer[10U] = (unsigned char)(m_rxLevel * 2.55F + 0.5F);

	buffer[11U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[12U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[13U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[14U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[15U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[16U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[17U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[18U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[19U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[20U] = (unsigned char)(txlevel * 2.55F + 0.5F);
	buffer[21U] = 0x00U;
	buffer[22U] = 0x00U;

	buffer[23U] = 0x00U;
	buffer[24U] = 0x00U;
	buffer[25U] = 0x00U;
	buffer[26U] = 0x00U;
	buffer[27U] = 0x00U;
	buffer[28U] = 0x00U;

	buffer[29U] = 1U;
	buffer[30U] = 0U;

	buffer[31U] = 134U;		// +6
	buffer[32U] = 0U;
	buffer[33U] = 0U;
	buffer[34U] = 0U;
	
	buffer[35U] = 0x00U;
	buffer[36U] = 0x00U;
	buffer[37U] = 0x00U;
	buffer[38U] = 0x00U;
	buffer[39U] = 0x00U;

	int ret = m_serial.write(buffer, 40U);
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

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setTXInvert()
{
	m_txInvert = !m_txInvert;

	::fprintf(stdout, "TX Invert: %s" EOL, m_txInvert ? "On" : "Off");

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setPTTInvert()
{
	m_pttInvert = !m_pttInvert;

	::fprintf(stdout, "PTT Invert: %s" EOL, m_pttInvert ? "On" : "Off");

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDebug()
{
	m_debug = !m_debug;

	::fprintf(stdout, "Modem debug: %s" EOL, m_debug ? "On" : "Off");

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDMRDeviation()
{
	m_mode = STATE_DMRCAL;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "DMR Deviation Mode (Set to 2.75Khz Deviation)" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setFMDeviation()
{
	switch (m_mode) {
	case STATE_FMCAL10K:
		m_mode = STATE_FMCAL12K;
		 ::fprintf(stdout, "FM 12.5Khz channel spacing with 2.50Khz Deviation (1039.5hz)" EOL);
		break;

	case STATE_FMCAL12K:
		m_mode = STATE_FMCAL15K;
		::fprintf(stdout, "FM 15Khz channel spacing with 3.0Khz Deviation (1247hz)" EOL);
		break;

	case STATE_FMCAL15K:
		m_mode = STATE_FMCAL20K;
		::fprintf(stdout, "FM 20Khz channel spacing with 4.0Khz Deviation (1663hz)" EOL);
		break;

	case STATE_FMCAL20K:
		m_mode = STATE_FMCAL25K;
		::fprintf(stdout, "FM 25Khz channel spacing with 5.0Khz Deviation (2079hz)" EOL);
		break;

	case STATE_FMCAL25K:
		m_mode = STATE_FMCAL30K;
		::fprintf(stdout, "FM 30Khz channel spacing with 6.0Khz Deviation (2495hz)" EOL);
		break;

	default:
		m_mode = STATE_FMCAL10K;
		::fprintf(stdout, "FM 10Khz channel spacing with 2.30Khz Deviation (956hz)" EOL);
		break;
	}

	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setLowFrequencyCal()
{
	m_mode = STATE_LFCAL;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "DMR Low Frequency Mode (80 Hz square wave)" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDMRCal1K()
{
	m_mode = STATE_DMRCAL1K;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "DMR Duplex 1031 Hz Test Pattern (TS2 CC1 ID1 TG9)" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDMRDMO1K()
{
	if (m_transmit && (m_hwType == HWT_MMDVM_HS)) {
		::fprintf(stdout, "First turn off the transmitter" EOL);
		return false;
	} else {
		m_mode = STATE_DMRDMO1K;
		m_carrier = false;
		m_duplex = true;
		m_dstarEnabled = false;
		m_dmrEnabled = false;
		m_ysfEnabled = false;
		m_p25Enabled = false;
		m_nxdnEnabled = false;
		m_m17Enabled = false;
		m_pocsagEnabled = false;
		m_fmEnabled = false;
		m_ax25Enabled = false;

		::fprintf(stdout, "DMR Simplex 1031 Hz Test Pattern (CC1 ID1 TG9)" EOL);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}
}

bool CMMDVMCal::setP25Cal1K()
{
	m_mode = STATE_P25CAL1K;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "P25 1011 Hz Test Pattern (NAC293 ID1 TG1)" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setNXDNCal1K()
{
	m_mode = STATE_NXDNCAL1K;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "NXDN 1031 Hz Test Pattern (RAN1 ID1 TG1)" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setPOCSAGCal()
{
	m_mode = STATE_POCSAGCAL;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "POCSAG 600 Hz Test Pattern" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDSTARBER_FEC()
{
	m_mode = STATE_DSTAR;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = true;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (FEC) for D-Star" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDMRBER_FEC()
{
	m_mode = STATE_DMR;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = true;
	m_dmrBERFEC = true;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (FEC) for DMR Simplex" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDMRBER_1K()
{
	m_mode = STATE_DMR;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = true;
	m_dmrBERFEC = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (1031 Hz Test Pattern) for DMR Simplex" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setYSFBER_FEC()
{
	m_mode = STATE_YSF;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = true;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (FEC) for YSF" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setP25BER_FEC()
{
	m_mode = STATE_P25;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = true;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (FEC) for P25" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setNXDNBER_FEC()
{
	m_mode = STATE_NXDN;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = true;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "BER Test Mode (FEC) for NXDN" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setDSTAR()
{
	m_mode = STATE_DSTARCAL;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "D-Star Mode" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setRSSI()
{
	m_mode = STATE_RSSICAL;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "RSSI Mode" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setM17Cal()
{
	m_mode = STATE_M17CAL;
	m_carrier = false;
	m_duplex = false;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "M17 Preamble Test Pattern" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setIntCal()
{
	m_mode = STATE_INTCAL;
	m_carrier = false;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "Interrupt Counter Mode" EOL);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setCarrier()
{
	m_mode = STATE_DMRCAL;
	m_carrier = true;
	m_duplex = true;
	m_dstarEnabled = false;
	m_dmrEnabled = false;
	m_ysfEnabled = false;
	m_p25Enabled = false;
	m_nxdnEnabled = false;
	m_m17Enabled = false;
	m_pocsagEnabled = false;
	m_fmEnabled = false;
	m_ax25Enabled = false;

	::fprintf(stdout, "Carrier Only Mode: %u Hz" EOL, m_frequency);

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setEnterFreq()
{
	char buff[256U];

	::fprintf(stdout, "Enter frequency (current %u Hz):" EOL, m_frequency);

	m_console.close();

	if (std::fgets(buff, 256, stdin) != NULL ) {

		unsigned long int freq = std::strtoul(buff, NULL, 10);
	
		if (freq >= 100000000U && freq <= 999999999U) {
			m_frequency = (unsigned int)freq;
			m_startfrequency = m_frequency;
			::fprintf(stdout, "New frequency: %u Hz" EOL, m_frequency);
			setFrequency();
		} else
			::fprintf(stdout, "Not valid frequency" EOL);
	}

	m_console.open();

	displayHelp_MMDVM_HS();

	switch (m_version) {
	case 1U:
		return writeConfig1(m_txLevel, m_debug);
	case 2U:
		return writeConfig2(m_txLevel, m_debug);
	default:
		return false;
	}
}

bool CMMDVMCal::setRXLevel(int incr)
{
	if (incr > 0 && m_rxLevel < 100.0F) {
		m_rxLevel += 0.5F;
		::fprintf(stdout, "RX Level: %.1f%%" EOL, m_rxLevel);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	if (incr < 0 && m_rxLevel > 0.0F) {
		m_rxLevel -= 0.5F;
		::fprintf(stdout, "RX Level: %.1f%%" EOL, m_rxLevel);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	return true;
}

bool CMMDVMCal::setTXLevel(int incr)
{
	if (!m_carrier) {
		if (incr > 0 && m_txLevel < 100.0F) {
			m_txLevel += 0.5F;
			::fprintf(stdout, "TX Level: %.1f%%" EOL, m_txLevel);

			switch (m_version) {
			case 1U:
				return writeConfig1(m_txLevel, m_debug);
			case 2U:
				return writeConfig2(m_txLevel, m_debug);
			default:
				return false;
			}
		}

		if (incr < 0 && m_txLevel > 0.0F) {
			m_txLevel -= 0.5F;
			::fprintf(stdout, "TX Level: %.1f%%" EOL, m_txLevel);

			switch (m_version) {
			case 1U:
				return writeConfig1(m_txLevel, m_debug);
			case 2U:
				return writeConfig2(m_txLevel, m_debug);
			default:
				return false;
			}
		}
	}

	return true;
}

bool CMMDVMCal::setFreq(int incr)
{
	if (incr > 0) {
		m_frequency += m_step;
		::fprintf(stdout, "TX frequency: %u" EOL, m_frequency);
		setFrequency();

		float txLevel = 0.0F;
		if (!m_carrier)
			txLevel = m_txLevel;

		switch (m_version) {
		case 1U:
			return writeConfig1(txLevel, m_debug);
		case 2U:
			return writeConfig2(txLevel, m_debug);
		default:
			return false;
		}
	}

	if (incr < 0) {
		m_frequency -= m_step;
		::fprintf(stdout, "TX frequency: %u" EOL, m_frequency);
		setFrequency();

		float txLevel = 0.0F;
		if (!m_carrier)
			txLevel = m_txLevel;

		switch (m_version) {
		case 1U:
			return writeConfig1(txLevel, m_debug);
		case 2U:
			return writeConfig2(txLevel, m_debug);
		default:
			return false;
		}
	}

	return true;
}

bool CMMDVMCal::setStepFreq()
{
	char buff[256U];

	::fprintf(stdout, "Enter frequency step (current %u Hz):" EOL, m_step);

	m_console.close();

	if (std::fgets(buff, 256, stdin) != NULL ) {

		unsigned long int freq = std::strtoul(buff, NULL, 10);
	
		if (freq >= 10U && freq <= 25000U) {
			m_step = (unsigned int)freq;
			::fprintf(stdout, "New frequency step: %u Hz" EOL, m_step);
		}
		else
			::fprintf(stdout, "Not valid frequency step" EOL);
	}

	m_console.open();

	displayHelp_MMDVM_HS();

	return true;
}

bool CMMDVMCal::setPower(int incr)
{
	if (incr > 0 && m_power < 100.0F) {
		m_power += 1.0F;
		::fprintf(stdout, "RF power: %.1f%%" EOL, m_power);
		setFrequency();

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	if (incr < 0 && m_power > 0.0F) {
		m_power -= 1.0F;
		::fprintf(stdout, "RF power: %.1f%%" EOL, m_power);
		setFrequency();

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	return true;
}

bool CMMDVMCal::setTXDCOffset(int incr)
{
	if (incr > 0 && m_txDCOffset < 127) {
		m_txDCOffset++;
		::fprintf(stdout, "TX DC Offset: %d" EOL, m_txDCOffset);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	if (incr < 0 && m_txDCOffset > -128) {
		m_txDCOffset--;
		::fprintf(stdout, "TX DC Offset: %d" EOL, m_txDCOffset);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	return true;
}

bool CMMDVMCal::setRXDCOffset(int incr)
{
	if (incr > 0 && m_rxDCOffset < 127) {
		m_rxDCOffset++;
		::fprintf(stdout, "RX DC Offset: %d" EOL, m_rxDCOffset);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
	}

	if (incr < 0 && m_rxDCOffset > -128) {
		m_rxDCOffset--;
		::fprintf(stdout, "RX DC Offset: %d" EOL, m_rxDCOffset);

		switch (m_version) {
		case 1U:
			return writeConfig1(m_txLevel, m_debug);
		case 2U:
			return writeConfig2(m_txLevel, m_debug);
		default:
			return false;
		}
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

	if (m_transmit)
		::fprintf(stdout, "Set transmitter ON" EOL);
	else
		::fprintf(stdout, "Set transmitter OFF" EOL);

	return true;
}

void CMMDVMCal::displayModem(const unsigned char *buffer, unsigned int length)
{
	if (buffer[2U] == MMDVM_GET_STATUS) {
		bool adcOverflow = (buffer[5U] & 0x02U) == 0x02U;
		if (adcOverflow)
			::fprintf(stderr, "MMDVM ADC levels have overflowed" EOL);
		bool rxOverflow = (buffer[5U] & 0x04U) == 0x04U;
		if (rxOverflow)
			::fprintf(stderr,"MMDVM RX buffer has overflowed" EOL);
		bool txOverflow = (buffer[5U] & 0x08U) == 0x08U;
		if (txOverflow)
			::fprintf(stderr,"MMDVM TX buffer has overflowed" EOL);
		bool dacOverflow = (buffer[5U] & 0x20U) == 0x20U;
		if (dacOverflow)
			::fprintf(stderr,"MMDVM DAC levels have overflowed" EOL);
	} else if (buffer[2U] == MMDVM_CAL_DATA) {
		bool  inverted = (buffer[3U] == 0x80U);
		short high = buffer[4U] << 8 | buffer[5U];
		short low = buffer[6U] << 8 | buffer[7U];
		short diff = high - low;
		short centre = (high + low) / 2;
		::fprintf(stdout, "Levels: inverted: %s, max: %d, min: %d, diff: %d, centre: %d" EOL, inverted ? "yes" : "no", high, low, diff, centre);
	} else if (buffer[2U] == MMDVM_RSSI_DATA) {
		unsigned short max = buffer[3U] << 8 | buffer[4U];
		unsigned short min = buffer[5U] << 8 | buffer[6U];
		unsigned short ave = buffer[7U] << 8 | buffer[8U];
		::fprintf(stdout, "RSSI: max: %u, min: %u, ave: %u" EOL, max, min, ave);
	} else if (buffer[2U] == MMDVM_DEBUG1) {
		::fprintf(stdout, "Debug: %.*s" EOL, length - 3U, buffer + 3U);
	} else if (buffer[2U] == MMDVM_DEBUG2) {
		short val1 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d" EOL, length - 5U, buffer + 3U, val1);
	} else if (buffer[2U] == MMDVM_DEBUG3) {
		short val1 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val2 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d" EOL, length - 7U, buffer + 3U, val1, val2);
	} else if (buffer[2U] == MMDVM_DEBUG4) {
		short val1 = (buffer[length - 6U] << 8) | buffer[length - 5U];
		short val2 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val3 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d %d" EOL, length - 9U, buffer + 3U, val1, val2, val3);
	} else if (buffer[2U] == MMDVM_DEBUG5) {
		short val1 = (buffer[length - 8U] << 8) | buffer[length - 7U];
		short val2 = (buffer[length - 6U] << 8) | buffer[length - 5U];
		short val3 = (buffer[length - 4U] << 8) | buffer[length - 3U];
		short val4 = (buffer[length - 2U] << 8) | buffer[length - 1U];
		::fprintf(stdout, "Debug: %.*s %d %d %d %d" EOL, length - 11U, buffer + 3U, val1, val2, val3, val4);
	} else if (buffer[2U] == MMDVM_DSTAR_HEADER || buffer[2U] == MMDVM_DSTAR_DATA || buffer[2U] == MMDVM_DSTAR_LOST || buffer[2U] == MMDVM_DSTAR_EOT) {
		m_ber.DSTARFEC(buffer + 3U, buffer[2U]);
	} else if (buffer[2U] == MMDVM_DMR_DATA1 || buffer[2U] == MMDVM_DMR_DATA2) {
		if (m_dmrBERFEC)
			m_ber.DMRFEC(buffer + 4U, buffer[3]);
		else
			m_ber.DMR1K(buffer + 4U, buffer[3]);
	} else if (buffer[2U] == MMDVM_YSF_DATA) {
		m_ber.YSFFEC(buffer + 4U);
	} else if (buffer[2U] == MMDVM_P25_HDR || buffer[2U] == MMDVM_P25_LDU) {
		m_ber.P25FEC(buffer + 4U);
	} else if (buffer[2U] == MMDVM_NXDN_DATA) {
		m_ber.NXDNFEC(buffer + 4U, buffer[3U]);
	} else if (m_hwType == HWT_MMDVM && m_mode != STATE_DMR && m_mode != STATE_P25 && m_mode != STATE_NXDN) {
		CUtils::dump("Response", buffer, length);
	}
}

bool CMMDVMCal::setFrequency()
{
	unsigned char buffer[16U];

	buffer[0U]  = MMDVM_FRAME_START;

	buffer[1U]  = 13U;

	buffer[2U]  = MMDVM_SET_FREQ;

	buffer[3U]  = 0x00U;

	buffer[4U]  = (m_frequency >> 0) & 0xFFU;
	buffer[5U]  = (m_frequency >> 8) & 0xFFU;
	buffer[6U]  = (m_frequency >> 16) & 0xFFU;
	buffer[7U]  = (m_frequency >> 24) & 0xFFU;

	buffer[8U]  = (m_frequency >> 0) & 0xFFU;
	buffer[9U]  = (m_frequency >> 8) & 0xFFU;
	buffer[10U] = (m_frequency >> 16) & 0xFFU;
	buffer[11U] = (m_frequency >> 24) & 0xFFU;

	buffer[12U]  = (unsigned char)(m_power * 2.55F + 0.5F);

	int ret = m_serial.write(buffer, 13U);
	if (ret != 13U)
		return false;

	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				::fprintf(stderr, "The MMDVM is not responding to the SET_FREQ command" EOL);
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		::fprintf(stderr, "Received a NAK to the SET_FREQ command from the modem, %u" EOL, m_buffer[4U]);
		return false;
	}

	return true;
}

bool CMMDVMCal::getStatus()
{
	unsigned char buffer[16U];
	buffer[0U]  = MMDVM_FRAME_START;
	buffer[1U]  = 3U;
	buffer[2U]  = MMDVM_GET_STATUS;

	int ret = m_serial.write(buffer, 3U);
	if (ret != 3)
		return false;
	unsigned int count = 0U;
	RESP_TYPE_MMDVM resp;
	do {
		sleep(10U);

		resp = getResponse();
		if (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK) {
			count++;
			if (count >= MAX_RESPONSES) {
				::fprintf(stderr, "The MMDVM is not responding to the GET_STATUS command" EOL);
				return false;
			}
		}
	} while (resp == RTM_OK && m_buffer[2U] != MMDVM_ACK && m_buffer[2U] != MMDVM_NAK);

	if (resp == RTM_OK && m_buffer[2U] == MMDVM_NAK) {
		::fprintf(stderr, "Received a NAK to the GET_STATUS command from the modem, %u" EOL, m_buffer[4U]);
		return false;
	}

	return true;
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

	m_offset = 0;

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
