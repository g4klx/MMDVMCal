/*
 * EEPROMData.h
 *
 *  Created on: Apr 5, 2023
 *      Author: lucas
 */

#ifndef EEPROMDATA_H_
#define EEPROMDATA_H_

#define EEPROMDATA_UHF_SIGNATURE_ADDRESS		0x00
#define EEPROMDATA_UHF_TXOFFSET_ADDRESS			0x02
#define EEPROMDATA_UHF_RXOFFSET_ADDRESS			0x04
#define EEPROMDATA_UHF_RESERVED_ADDRESS			0x06
#define EEPROMDATA_UHF_CRC_ADDRESS				0x07

#define EEPROMDATA_VHF_SIGNATURE_ADDRESS		0x08
#define EEPROMDATA_VHF_TXOFFSET_ADDRESS			0x0A
#define EEPROMDATA_VHF_RXOFFSET_ADDRESS			0x0C
#define EEPROMDATA_VHF_RESERVED_ADDRESS			0x0E
#define EEPROMDATA_VHF_CRC_ADDRESS				0x0F

#include "24cXX.h"

class CEEPROMData {
public:
	CEEPROMData();
	~CEEPROMData();
	int readData(unsigned char *data, uint16_t addr, unsigned int length, unsigned int timeout);
	int writeData(unsigned char *data, uint16_t addr, unsigned int length, unsigned int timeout);
	int getRxOffsetVHF();
	int getTxOffsetVHF();
	int getRxOffsetUHF();
	int getTxOffsetUHF();
	unsigned char readCRC8UHF();
	unsigned char readCRC8VHF();
	int setRxOffsetVHF(int freq);
	int setTxOffsetVHF(int freq);
	int setRxOffsetUHF(int freq);
	int setTxOffsetUHF(int freq);
	int writeCRC8UHF();
	int writeCRC8VHF();
	int readOffsetData(int &rxVHF, int &txVHF, int &rxUHF, int &txUHF);
	int writeOffsetData();
	bool checkData();
	bool checkDetected();

private:
	int writeSignature(int addr);
	bool readSignature(int addr);
	unsigned char crc8(unsigned char *data, unsigned int length);	// Based on code from https://github.com/bxparks/AceCRC
	int readInt(int addr);
	int writeInt(int data, int addr, unsigned int timeout);

	const unsigned int 	m_signature = 0x5a0f;
	int 				m_txOffsetUHF;
	int 				m_rxOffsetUHF;
	unsigned char 		m_reservedUHF;
	unsigned char 		m_crc8UHF;
	int 				m_txOffsetVHF;
	int 				m_rxOffsetVHF;
	unsigned char 		m_reservedVHF;
	unsigned char 		m_crc8VHF;
	C24CXX 				*m_EEPROM;
	bool 				m_EEPROMDetected;
};


#endif /* EEPROMDATA_H_ */
