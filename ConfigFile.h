/*
 * ConfigFile.h
 *
 *  Created on: May 1, 2023
 *      Author: lucas
 */

#ifndef CONFIGFILE_H_
#define CONFIGFILE_H_

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>

#define CONFIG_FILE_PATH	"/etc/mmdvmhost"

class ConfigFile {
public:
	ConfigFile();
	~ConfigFile();
	int getRxOffset();
	int getTxOffset();
	unsigned int getRxFrequency();
	unsigned int getTxFrequency();
	void setRxOffset(int n);
	void setTxOffset(int n);
	void setRxFrequency(unsigned int n);
	void setTxFrequency(unsigned int n);
	int readConfigData();		// Read from file and store in private vars
	int readFrequencies();
	int writeOffsets();		// Write what is stored in private vars to file
	int writeFrequencies();
	bool checkFile();

private:
	int 			m_rxOffset;
	int 			m_txOffset;
	unsigned int 	m_rxFrequency;
	unsigned int 	m_txFrequency;
	FILE 			*m_file;
	int 			m_fd;
	char 			m_filename[100];
	bool 			m_fileExists;
};



#endif /* CONFIGFILE_H_ */
