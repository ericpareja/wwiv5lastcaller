ODOORS="../OpenDoors"
WWIVSRC="../wwiv-stock/"
WWIVBUILD="../wwiv-stock-build/"

xw5-ilc: main.o Program.o INIReader.o
	g++ --std=c++17 -o xw5-ilc ../inih/ini.o INIReader.o main.o Program.o $(WWIVBUILD)bbs/libbbs_lib.a $(WWIVBUILD)sdk/libsdk.a $(WWIVBUILD)core/libcore.a $(ODOORS)/libs-Linux/libODoors.a $(WWIVBUILD)vcpkg_installed/x64-linux/lib/libfmt.a

INIReader.o: INIReader.cpp
	g++ --std=c++17 -c INIReader.cpp

main.o: main.cpp
	g++ --std=c++17 -I$(ODOORS) -c main.cpp

Program.o: Program.cpp
	g++ --std=c++17 -I$(WWIVSRC) -I$(WWIVBUILD)vcpkg_installed/x64-linux/include/ -I$(ODOORS) -c Program.cpp

clean:
	rm xw5-ilc INIReader.o main.o Program.o

push: xw5-ilc
	scp xw5-ilc bbs:

