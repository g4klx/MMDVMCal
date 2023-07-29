all:	MMDVMCal

LD  = c++
CXX = c++
CC = cc

CXXFLAGS = -O2 -Wall -std=c++0x
CFLAGS = -O2 -Wall
LDFLAGS =

# Use the following line if compiling on a platform that supports I2C.
# LIBS = -li2c

MMDVMCal:	24cXX.o BERCal.o ConfigFile.o CRC.o EEPROMData.o Hamming.o Golay24128.o P25Utils.o MMDVMCal.o NXDNLICH.o SerialController.o SerialPort.o smbus.o Console.o Utils.o YSFConvolution.o YSFFICH.o
		$(CXX) $(LDFLAGS) -o MMDVMCal 24cXX.o BERCal.o ConfigFile.o CRC.o EEPROMData.o  Hamming.o Golay24128.o P25Utils.o MMDVMCal.o NXDNLICH.o SerialController.o SerialPort.o smbus.o Console.o Utils.o YSFConvolution.o YSFFICH.o $(LIBS)

BERCal.o:	BERCal.cpp BERCal.h Golay24128.h Utils.h
		$(CXX) $(CXXFLAGS) -c BERCal.cpp

ConfigFile.o:	ConfigFile.cpp ConfigFile.h
		$(CXX) $(CXXFLAGS) -c ConfigFile.cpp

CRC.o:		CRC.cpp CRC.h
		$(CXX) $(CXXFLAGS) -c CRC.cpp

24cXX.o:	24cXX.cpp 24cXX.h smbus.h
		$(CXX) $(CXXFLAGS) -c 24cXX.cpp 

EEPROMData.o:	EEPROMData.cpp EEPROMData.h 24cXX.h
		$(CXX) $(CXXFLAGS) -c EEPROMData.cpp

Hamming.o:	Hamming.cpp Hamming.h
		$(CXX) $(CXXFLAGS) -c Hamming.cpp

Golay24128.o:	Golay24128.cpp Golay24128.h
		$(CXX) $(CXXFLAGS) -c Golay24128.cpp

P25Utils.o:	P25Utils.cpp P25Utils.h
		$(CXX) $(CXXFLAGS) -c P25Utils.cpp

MMDVMCal.o:	MMDVMCal.cpp MMDVMCal.h SerialController.h Console.h Utils.h
		$(CXX) $(CXXFLAGS) -c MMDVMCal.cpp

NXDNLICH.o:	NXDNLICH.cpp NXDNLICH.h NXDNDefines.h
		$(CXX) $(CXXFLAGS) -c NXDNLICH.cpp

SerialController.o:	SerialController.cpp SerialController.h
		$(CXX) $(CXXFLAGS) -c SerialController.cpp

SerialPort.o:	SerialPort.cpp SerialPort.h
		$(CXX) $(CXXFLAGS) -c SerialPort.cpp

smbus.o:	smbus.c smbus.h
		$(CC) $(CFLAGS) -c smbus.c

Console.o:	Console.cpp Console.h
		$(CXX) $(CXXFLAGS) -c Console.cpp

Utils.o:	Utils.cpp Utils.h
		$(CXX) $(CXXFLAGS) -c Utils.cpp

YSFConvolution.o:	YSFConvolution.cpp YSFConvolution.h
		$(CXX) $(CXXFLAGS) -c YSFConvolution.cpp

YSFFICH.o:	YSFFICH.cpp CRC.h Golay24128.h YSFConvolution.h YSFDefines.h YSFFICH.h
		$(CXX) $(CXXFLAGS) -c YSFFICH.cpp


MMDVMCal.o: GitVersion.h FORCE

.PHONY: GitVersion.h

FORCE:

install:
		install -m 755 MMDVMCal /usr/local/bin/

clean:
		$(RM) *.o *.bak *~ MMDVMCal

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif

