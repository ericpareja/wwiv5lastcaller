xw5-ilc: main.o Program.o
	g++ --std=c++23 -o xw5-ilc ../inih/ini.o INIReader.o main.o Program.o /u2/xenos/wwv/github/wwiv-stock-build/bbs/libbbs_lib.a /u2/xenos/wwv/github/wwiv-stock-build/sdk/libsdk.a /u2/xenos/wwv/github/wwiv-stock-build/core/libcore.a /u2/xenos/wwv/github/odoors/libs-Linux/libODoors.a

INIReader.o: INIReader.cpp
	g++ --std=c++23 -c INIReader.cpp

main.o: main.cpp
	g++ --std=c++23 -I/u2/xenos/wwv/github/odoors/ -c main.cpp

Program.o: Program.cpp
	g++ --std=c++23 -I/u2/xenos/wwv/github/wwiv-stock/ -I/u2/xenos/wwv/github/wwiv-stock-build/vcpkg_installed/x64-linux/include/ -I/u2/xenos/wwv/github/odoors/ -c Program.cpp

