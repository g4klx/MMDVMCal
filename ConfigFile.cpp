/*
 * ConfigFile.cpp
 *
 *  Created on: May 1, 2023
 *      Author: lucas
 */


#include "ConfigFile.h"


ConfigFile::ConfigFile()
{
#ifdef __linux__
	::sprintf(m_filename, "%s", CONFIG_FILE_PATH);
	int n = readConfigData();
	if (n < 0) {
		::fprintf(stderr, "Error opening configuration file.\n");
		m_fileExists = false;
	} else {
		m_fileExists = true;
	}
#else
	m_fileExists = false;
#endif
}

ConfigFile::~ConfigFile()
{

}

int ConfigFile::getRxOffset()
{
	return m_rxOffset;
}

int ConfigFile::getTxOffset()
{
	return m_txOffset;
}

unsigned int ConfigFile::getRxFrequency()
{
	return m_rxFrequency;
}

unsigned int ConfigFile::getTxFrequency()
{
	return m_txFrequency;
}

void ConfigFile::setRxOffset(int n)
{
	m_rxOffset = n;
}

void ConfigFile::setTxOffset(int n)
{
	m_txOffset = n;
}

void ConfigFile::setRxFrequency(unsigned int n)
{
	m_rxFrequency = n;
}

void ConfigFile::setTxFrequency(unsigned int n)
{
	m_txFrequency = n;
}

int ConfigFile::readConfigData()
{
	// Mechanism for this borrowed from Conf.cpp in the MMDVMHost project

	char curLine[500];
	size_t len;
	bool foundSection = false;

	// Debug
	//::fprintf(stdout, "Opening configuration file...\n");

	m_file = ::fopen(m_filename, "rt");
	if (m_file == NULL) {
		return -1;
	}

	// Debug
	//::fprintf(stdout, "Reading configuration file...\n");

	while(::fgets(curLine, 500, m_file) != NULL) {

		// Found a comment, skip it
		if (curLine[0] == '#') {
			continue;
		}

		// Found a section header, is it one of the right ones?
		if (curLine[0] == '[') {
			if ((::strncmp(curLine, "[Modem]", 7) == 0) || (::strncmp(curLine, "[Info]", 6) == 0)) {
				foundSection = true;
			} else {
				foundSection = false;
			}
			continue;
		}

		if (foundSection == false) continue;

		// Extract the key and value of current line if there is one
		char* key = ::strtok(curLine, " \t=\r\n");
		if (key == NULL)
			continue;

		char* value = ::strtok(NULL, "\r\n");
		if (value == NULL)
			continue;

		// Remove quotes from the value
		len = ::strlen(value);
		if (len > 1U && *value == '"' && value[len - 1U] == '"') {
			value[len - 1U] = '\0';
			value++;
		} else {
			char *p;

			// if value is not quoted, remove after # (to make comment)
			if ((p = strchr(value, '#')) != NULL)
				*p = '\0';

			// remove trailing tab/space
			for (p = value + ::strlen(value) - 1U; p >= value && (*p == '\t' || *p == ' '); p--)
				*p = '\0';
		}

		// Debug
		//::fprintf(stdout, "Key: %s, Value: %s\n", key, value);

		// If we're in the right section, extract the data
		if (::strcmp(key, "RXOffset") == 0) {
			m_rxOffset = ::atoi(value);
		} else if (::strcmp(key, "TXOffset") == 0) {
			m_txOffset = ::atoi(value);
		} else if (::strcmp(key, "RXFrequency") == 0) {
			m_rxFrequency = ::strtoul(value, NULL, 0);
		} else if (::strcmp(key, "TXFrequency") == 0) {
			m_txFrequency = ::strtoul(value, NULL, 0);
		}
	}

	// Debug
	::fprintf(stdout, "Read offset values from config file.\nrxOffset = %d, txOffset = %d, rxFrequency = %u, TxFrequency = %u\n", m_rxOffset, m_txOffset, m_rxFrequency, m_txFrequency);

	::fclose(m_file);

	return 0;
}

int ConfigFile::writeOffsets()
{
	// Debug
	//::fprintf(stdout, "Opening configuration file...\n");

	// Open the actual config file
	m_file = ::fopen(m_filename, "r+");
	if (m_file == NULL) {
		::fprintf(stderr, "Error opening configuration file.\n");
		return -1;
	}

	// Open a temporary file for writing in
	char tempFilename[110];
	::strcpy(tempFilename, m_filename);
	::strcat(tempFilename, "-tmp");

	//::fprintf(stdout, "Creating temporary configuration file: %s\n", tempFilename);
	FILE *tempFile = ::fopen(tempFilename, "w+");
	if (tempFile == NULL) {
		::fprintf(stderr, "Error opening temp configuration file.\n");
		return -1;
	}

	char curLine[500];
	char curLineTemp[500];
	bool foundSection = false;

	while (::fgets(curLine, 500, m_file) != NULL) {

		// Debug
		//::fprintf(stdout, "%s", curLine);

		// Found a section header, is it the right one?
		if (curLine[0] == '[') {
			if (::strncmp(curLine, "[Modem]", 7) == 0) {
				foundSection = true;
			} else {
				foundSection = false;
			}

		}

		// We're not in the section, just copy the line
		if (foundSection == false) {
			::fprintf(tempFile, "%s", curLine);
			continue;
		}

		::strcpy(curLineTemp, curLine);

		// Extract the key and value of current line if there is one
		char* key = ::strtok(curLineTemp, " \t=\r\n");
		// Simply copy the line and move on if no key was read
		if (key == NULL) {
			::fprintf(tempFile, "%s", curLine);
			continue;
		}

		// Check if we need to overwrite the value or simply copy the line
		if (::strcmp(key, "RXOffset") == 0) {
			::fprintf(tempFile, "RXOffset=%d\n", m_rxOffset);
		} else if (::strcmp(key, "TXOffset") == 0) {
			::fprintf(tempFile, "TXOffset=%d\n", m_txOffset);
		} else {
			::fprintf(tempFile, "%s", curLine);
		}
	}

	// Close the files since we're done reading/writing them
	::fclose(tempFile);
	::fclose(m_file);

	// Overwrite the actual file
	::remove(m_filename);
	::rename(tempFilename, m_filename);

	return 0;
}

bool ConfigFile::checkFile()
{
	return m_fileExists;
}
