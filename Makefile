all:	MMDVMCal

LD  = g++
CXX = g++

CXXFLAGS = -O2 -Wall

MMDVMCal:	MMDVMCal.o SerialController.o Console.o Utils.o
		$(LD) $(LDFLAGS) -o MMDVMCal MMDVMCal.o SerialController.o Console.o Utils.o $(LIBS)

MMDVMCal.o:	MMDVMCal.cpp MMDVMCal.h SerialController.h Console.h Utils.h
		$(CXX) $(CXXFLAGS) -c MMDVMCal.cpp

SerialController.o:	SerialController.cpp SerialController.h
		$(CXX) $(CXXFLAGS) -c SerialController.cpp

Console.o:	Console.cpp Console.h
		$(CXX) $(CXXFLAGS) -c Console.cpp

Utils.o:	Utils.cpp Utils.h
		$(CXX) $(CXXFLAGS) -c Utils.cpp

clean:
		rm -f *.o *.bak *~ MMDVMCal
