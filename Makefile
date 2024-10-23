xw5-ilc: main.o Program.o INIReader.o
	g++ --std=c++17 -o xw5-ilc ../inih/ini.o INIReader.o main.o Program.o ../wwiv-stock-build/bbs/libbbs_lib.a ../wwiv-stock-build/sdk/libsdk.a ../wwiv-stock-build/core/libcore.a ../odoors/libs-Linux/libODoors.a ../wwiv-stock-build/vcpkg_installed/x64-linux/lib/libfmt.a

INIReader.o: INIReader.cpp
	g++ --std=c++17 -c INIReader.cpp

main.o: main.cpp
	g++ --std=c++17 -I../odoors/ -c main.cpp

Program.o: Program.cpp
	g++ --std=c++17 -I../wwiv-stock/ -I../wwiv-stock-build/vcpkg_installed/x64-linux/include/ -I../odoors/ -c Program.cpp

clean:
	rm xw5-ilc INIReader.o main.o Program.o
