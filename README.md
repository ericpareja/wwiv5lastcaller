This is xenos' WWIV 5 InterBBS Last Caller utility (xw5-ilc)

Currently consumes incoming InterBBS LastCaller rot47 posts from:

    fsxNet AREA: FSX_DAT
    WWIVnet Subtype: IBBSDAT hosted by @60

Note that this implementation of IBBS Last Caller does not delete
messages from the message base. It re-reads the entire message bases
to generate the IBBS last caller files for each network.

It will also post rot47 encoded ibbslastcall-data to the same SUB/Echo.

INSTALLING

1) To use this utility, copy the binary file, xw5-ilc, to the WWIV BBS
path. This is usually /home/wwiv/.

2) In your wwiv.ini file, add a section as follows:
```
[xw5-ilc]
WWIV Path = /home/wwiv
BBS Name = WWIV BBS
BBS Address = wwivbbs.org
Data Area = wwivdata
display = 10
dontshow = 255
```
Adjust to your own settings and taste.

Data Area is the FILENAME of the SUB that carries either the WWIVnet STYPE IBBSDAT or fsxNet Echo Area FSX_DAT.
display is the number of Last Caller entries you want to show.
dontshow is the minimum SL of users you don't want to appear in lastcallers.

3) In wwiv.ini again, edit a line for LOGON_CMD:

``
LOGON_CMD = /home/wwiv/xw5-ilc -D %C
``

This will run the program whenever a user logs on. We need to pass the value of the CHAIN.TXT file so that the program can pick up data from it.

That's it.

BUILDING

If you want to compile this from sources, you should have a similar
directory layout as below:

```
github/wwiv          # WWIV sources
github/wwiv-build    # WWIV build directory
github/inih          # INI Reader
github/OpenDoors     # OpenDoors library
github/wwiv5lastcaller # this project
```

To set this up:

```bash
mkdir github
cd github
git clone --recurse-submodules https://github.com/wwivbbs/wwiv
git clone https://github.com/benhoyt/inih
git clone https://github.com/RealDeuce/OpenDoors
git clone https://github.com/ericpareja/wwiv5lastcaller
mkdir wwiv-build
cd wwiv-build
export CC=gcc-12
export CXX=g++-12
export CPP=g++-12
../wwiv/cmake-config.sh
cmake --build . -j`nproc`
cd ../inih
make ini.o
cd ../OpenDoors
make
cd wwiv5lastcaller
make
```

This utility was inspired by xqtr's ilc for MysticBBS.

Email me: xenos@aliens.ph
Send me WWIVnet mail: 1@60
Send me fsxNet netmail: Xenos@21:4/147
                        Eric Pareja@21:4/147

https://github.com/ericpareja/wwiv5lastcaller