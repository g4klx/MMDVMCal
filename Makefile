all:	MMDVMCal_eeprom

LD  = c++
CXX = c++

CXXFLAGS = -O2 -Wall -std=c++0x -li2c
LDFLAGS = -li2c

MMDVMCal_eeprom:	24cXX.o BERCal.o ConfigFile.o CRC.o EEPROMData.o Hamming.o Golay24128.o P25Utils.o MMDVMCal.o NXDNLICH.o SerialController.o SerialPort.o Console.o Utils.o YSFConvolution.o YSFFICH.o
			$(CXX) $(LDFLAGS) -o MMDVMCal_eeprom 24cXX.o BERCal.o ConfigFile.o CRC.o EEPROMData.o  Hamming.o Golay24128.o P25Utils.o MMDVMCal.o NXDNLICH.o SerialController.o SerialPort.o Console.o Utils.o YSFConvolution.o YSFFICH.o $(LIBS)

BERCal.o:	BERCal.cpp BERCal.h Golay24128.h Utils.h
		$(CXX) $(CXXFLAGS) -c BERCal.cpp

ConfigFile.o:	ConfigFile.cpp ConfigFile.h
		$(CXX) $(CXXFLAGS) -c ConfigFile.cpp

CRC.o:		CRC.cpp CRC.h
		$(CXX) $(CXXFLAGS) -c CRC.cpp

24cXX.o:	24cXX.cpp 24cXX.h
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

Console.o:	Console.cpp Console.h
		$(CXX) $(CXXFLAGS) -c Console.cpp

Utils.o:	Utils.cpp Utils.h
		$(CXX) $(CXXFLAGS) -c Utils.cpp

YSFConvolution.o:	YSFConvolution.cpp YSFConvolution.h
		$(CXX) $(CXXFLAGS) -c YSFConvolution.cpp

YSFFICH.o:	YSFFICH.cpp CRC.h Golay24128.h YSFConvolution.h YSFDefines.h YSFFICH.h
		$(CXX) $(CXXFLAGS) -c YSFFICH.cpp

install:
		install -m 755 MMDVMCal_eeprom /usr/local/bin/

clean:
		rm -f *.o *.bak *~ MMDVMCal_eeprom
