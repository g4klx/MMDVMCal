/*
 * EEPROMData.cpp
 *
 *  Created on: Apr 5, 2023
 *      Author: lucas
 */

/*
 * 		EEPROM Data Organization (Big Endian):
 *
 * 		Address			Name				Length(bytes)			Data Type
 * 		0x00			UHF Signature		2						unsigned int
 * 		0x02			UHF Tx Offset		2						signed int
 * 		0x04			UHF Rx Offset		2						signed int
 * 		0x06			Reserved			1						unsigned char
 * 		0x07			UHF Offset CRC		1						unsigned char
 *
 * 		0x08			VHF Signature		2						unsigned int
 * 		0x0A			VHF Tx Offset		2						signed int
 * 		0x0C			VHF Rx Offset		2						signed int
 * 		0x0E			Reserved			1						unsigned char
 * 		0x0F			VHF Offset CRC		1						unsigned char
 */



#define EEPROM_ADDRESS		0x50
#define EEPROM_DEVICE		"/dev/i2c-1"

#include <cstdint>
#include "EEPROMData.h"

CEEPROMData::CEEPROMData() :
m_txOffsetUHF(0),
m_rxOffsetUHF(0),
m_reservedUHF(0),
m_crc8UHF(0U),
m_txOffsetVHF(0),
m_rxOffsetVHF(0),
m_reservedVHF(0),
m_crc8VHF(0U)
{
	m_EEPROM = new C24CXX(EEPROM_DEVICE, EEPROM_ADDRESS, ET_8BITADDR);

	if ((m_EEPROM->getFileDescriptor() < 0) || (m_EEPROM->eeprom_check() == false)) {
		::fprintf(stderr, "Onboard EEPROM not detected.\n");
		m_EEPROMDetected = false;
		m_txOffsetUHF = m_rxOffsetUHF = m_txOffsetVHF = m_rxOffsetVHF = 0;
		return;
	}

	::fprintf(stdout, "Onboard EEPROM detected.\n");
	m_EEPROMDetected = true;
	bool dataIsGood = checkData();
	if (!dataIsGood) {
		::fprintf(stderr, "EEPROM offset data either does not exist or is corrupted.\n");
	} else {
		//::fprintf(stdout, "EEPROM offset data detected and verified.\n");
		m_txOffsetUHF = readInt(EEPROMDATA_UHF_TXOFFSET_ADDRESS);
		m_rxOffsetUHF = readInt(EEPROMDATA_UHF_RXOFFSET_ADDRESS);
		m_txOffsetVHF = readInt(EEPROMDATA_VHF_TXOFFSET_ADDRESS);
		m_rxOffsetVHF = readInt(EEPROMDATA_VHF_RXOFFSET_ADDRESS);
	}
}

CEEPROMData::~CEEPROMData()
{
	//::fprintf(stdout, "Destroying CEEPROMData object...\n");
	delete m_EEPROM;
}

int CEEPROMData::readData(unsigned char *data, uint16_t addr,  unsigned int length, unsigned int timeout)
{
	if (m_EEPROMDetected == false) return -1;

	for (unsigned int i = 0; i < length; i++) {
		if (&data[i] != NULL) {
			data[i] = static_cast<unsigned char>(m_EEPROM->eeprom_read_byte(addr+i));
		}
	}

	return 0;
}

int CEEPROMData::writeData(unsigned char *data, uint16_t addr, unsigned int length, unsigned int timeout)
{
	if (m_EEPROMDetected == false) return -1;

	for (unsigned int i = 0; i < length; i++) {
		if (&data[i] != NULL) {
			m_EEPROM->eeprom_write_byte(addr + i, data[i]);
		}
		if (m_EEPROM->eeprom_wait_ready(timeout) < 0) {
			::printf("In C24CXX::eeprom_write: Writing timed out. Aborting write...\n");
			return -1;
		}
	}

	return 0;
}

int CEEPROMData::getRxOffsetVHF()
{
	return m_rxOffsetVHF;
}

int CEEPROMData::getTxOffsetVHF()
{
	return m_txOffsetVHF;
}

int CEEPROMData::getRxOffsetUHF()
{
	return m_rxOffsetUHF;
}

int CEEPROMData::getTxOffsetUHF()
{
	return m_txOffsetUHF;
}

unsigned char CEEPROMData::readCRC8UHF()
{
	return m_crc8UHF;
}

unsigned char CEEPROMData::readCRC8VHF()
{
	return m_crc8VHF;
}

int CEEPROMData::setRxOffsetVHF(int freq)
{
	m_rxOffsetVHF = freq;

	return 0;
}

int CEEPROMData::setTxOffsetVHF(int freq)
{
	m_txOffsetVHF = freq;

	return 0;
}

int CEEPROMData::setRxOffsetUHF(int freq)
{
	m_rxOffsetUHF = freq;

	return 0;
}

int CEEPROMData::setTxOffsetUHF(int freq)
{
	m_txOffsetUHF = freq;

	return 0;
}

int CEEPROMData::writeSignature(int addr)
{
	if (m_EEPROMDetected == false) return -1;

	unsigned char s0, s1;
	s0 = static_cast<uint16_t>(m_signature & 0x00FF);
	s1 = static_cast<uint16_t>(m_signature & 0xFF00) >> 8;
	//::fprintf(stdout, "Writing signature (0x%04X) at address 0x%02X\n", m_signature, addr);
	m_EEPROM->eeprom_write(addr, &s1, 1, 10);		// MSB
	m_EEPROM->eeprom_write(addr+1, &s0, 1, 10);		// LSB


	return 0;
}

bool CEEPROMData::readSignature(int addr)
{
	if (m_EEPROMDetected == false) return false;

	unsigned char sig[2];

	readData(sig, addr, 2, 10);
	//::fprintf(stdout, "Signature MSB: 0x%02X\nSignature LSB: 0x%02X\n", sig[0], sig[1]);

	if ((sig[0] == ((m_signature & 0xFF00) >> 8)) && (sig[1] == (m_signature & 0x00FF))) {
		return true;
	}

	return false;
}

unsigned char CEEPROMData::crc8(unsigned char *data, unsigned int length)
{
	unsigned char crc = 0;

	unsigned char crcTable[16] = { 0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
								   0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D
	};

	for (unsigned int i = 0; i < length; i++) {
		crc = (crc << 4) ^ crcTable[((crc >> 0) ^ data[i]) >> 4];
		crc = (crc << 4) ^ crcTable[((crc >> 4) ^ data[i]) & 0x0F];
	}

	//::fprintf(stdout, "Calculated CRC8: 0x%02X\n", crc);

	return crc;
}

int CEEPROMData::writeCRC8UHF()
{
	unsigned char offsetDataUHF[7];

	int n = readData(offsetDataUHF, EEPROMDATA_UHF_SIGNATURE_ADDRESS, 7, 10);
	m_crc8UHF = crc8(offsetDataUHF, sizeof(offsetDataUHF));

	return n;
}

int CEEPROMData::writeCRC8VHF()
{
	unsigned char offsetDataVHF[7];

	int n = readData(offsetDataVHF, EEPROMDATA_VHF_SIGNATURE_ADDRESS, 7, 10);
	m_crc8VHF = crc8(offsetDataVHF, sizeof(offsetDataVHF));

	return n;
}

int CEEPROMData::readOffsetData(int &txUHF, int &rxUHF, int &txVHF, int &rxVHF)
{

	txUHF = m_txOffsetUHF;
	rxUHF = m_rxOffsetUHF;
	txVHF = m_txOffsetVHF;
	rxVHF = m_rxOffsetVHF;

	return 0;
}

int CEEPROMData::writeOffsetData()
{
	if (m_EEPROMDetected == false) return -1;

	// Write the UHF signature bytes
	writeSignature(EEPROMDATA_UHF_SIGNATURE_ADDRESS);
	// Write the UHF offsets
	writeInt(m_txOffsetUHF, EEPROMDATA_UHF_TXOFFSET_ADDRESS, 10);
	writeInt(m_rxOffsetUHF, EEPROMDATA_UHF_RXOFFSET_ADDRESS, 10);
	// Write the UHF reserved byte
	m_EEPROM->eeprom_write(EEPROMDATA_UHF_RESERVED_ADDRESS, &m_reservedUHF, 1, 10);
	// Calculate the UHF CRC8
	writeCRC8UHF();
	// Write the UHF CRC8
	m_EEPROM->eeprom_write(EEPROMDATA_UHF_CRC_ADDRESS, &m_crc8UHF, 1, 10);

	// Write the VHF signature bytes
	writeSignature(EEPROMDATA_VHF_SIGNATURE_ADDRESS);
	// Write the VHF offsets
	writeInt(m_txOffsetVHF, EEPROMDATA_VHF_TXOFFSET_ADDRESS, 10);
	writeInt(m_rxOffsetVHF, EEPROMDATA_VHF_RXOFFSET_ADDRESS, 10);
	// Write the VHF reserved byte
	m_EEPROM->eeprom_write(EEPROMDATA_VHF_RESERVED_ADDRESS, &m_reservedVHF, 1, 10);
	// Calculate the VHF CRC8
	writeCRC8VHF();
	// Write the VHF CRC8
	m_EEPROM->eeprom_write(EEPROMDATA_VHF_CRC_ADDRESS, &m_crc8VHF, 1, 10);

	return 0;
}

bool CEEPROMData::checkData()
{
	// Check if data exists and is valid
	bool sigTestUHF = readSignature(EEPROMDATA_UHF_SIGNATURE_ADDRESS);
	bool sigTestVHF = readSignature(EEPROMDATA_VHF_SIGNATURE_ADDRESS);
	if (sigTestUHF && sigTestVHF) {
		//::fprintf(stdout, "UHF and VHF signatures detected.\n");
	} else {
		//::fprintf(stdout, "Error reading UHF and VHF signatures...\n");
		return false;
	}

	unsigned char uhfData[8], vhfData[8];
	readData(uhfData, EEPROMDATA_UHF_SIGNATURE_ADDRESS, 8, 10);
	readData(vhfData, EEPROMDATA_VHF_SIGNATURE_ADDRESS, 8, 10);
	unsigned char cu, cv;
	cu = crc8(uhfData, 8);
	cv = crc8(vhfData, 8);
	if ((cu == 0x00) && (cv == 0x00)) {
		//::fprintf(stdout, "UHF and VHF data passed CRC check.\n");
	} else {
		//::fprintf(stdout, "UHF and VHF data failed CRC check.\n");
		return false;
	}

	return true;
}

int CEEPROMData::readInt(int addr)
{
	int16_t n = 0;

	unsigned char n0 = 0;
	unsigned char n1 = 0;

	readData(&n0, addr+1, 1, 10);					// LSB
	readData(&n1, addr, 1, 10);						// MSB

	n |= static_cast<int16_t>(n0);
	n |= static_cast<int16_t>(n1 << 8);

	//::fprintf(stdout, "Read address 0x%02X: n = %d\n", addr, n);

	return n;
}

int CEEPROMData::writeInt(int data, int addr, unsigned int timeout)
{
	unsigned char n0, n1;

	n0 = static_cast<uint16_t>(data) & 0xFF;
	n1 = static_cast<uint16_t>(data >> 8) & 0xFF;

	// Debug
	//::fprintf(stdout, "Writing 0x%02X to address 0x%02X...\n", n0, addr + 1);
	//::fprintf(stdout, "Writing 0x%02X to address 0x%02X...\n", n1, addr);

	// Actually write the data
	m_EEPROM->eeprom_write(addr, &n1, 1, 10);			// MSB
	m_EEPROM->eeprom_write(addr+1, &n0, 1, 10);			// LSB

	return 0;
}

bool CEEPROMData::checkDetected()
{
	return m_EEPROMDetected;
}
