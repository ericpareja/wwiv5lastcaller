#include <core/datetime.h>
#include <core/log.h>
#include <sdk/config.h>
#include <sdk/net/networks.h>
#include <sdk/msgapi/message_api_wwiv.h>
#include <sdk/msgapi/msgapi.h>
#include <sdk/user.h>
#include <sdk/usermanager.h>
#include <sdk/vardec.h>
#include <OpenDoor.h>
#include "Program.h"
#include "../inih/cpp/INIReader.h"
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#define _stricmp strcasecmp
#endif

#define LASTON_TXT "laston.txt"

static wwiv::sdk::subboard_t default_sub(const std::string& fn) {
	wwiv::sdk::subboard_t sub{};
	sub.storage_type = 2;
	sub.filename = fn;
	return sub;
}

static std::optional<wwiv::sdk::subboard_t> find_sub(const wwiv::sdk::Subs& subs, const std::string& filename) {
	for (const auto& x : subs.subs()) {
		if (_stricmp(filename.c_str(), x.filename.c_str()) == 0) {
			return { x };
		}
	}
	return std::nullopt;
}

static std::string strip_annoying_stuff(std::string str) {
	std::stringstream ss;

	ss.str("");

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == '\r') {
			continue;
		}
		if (str[i] == '|') {
			i += 2;
			continue;
		}
		ss << str[i];
	}

	return ss.str();
}

std::string rot47(const std::string& s) {
        std::string ret;
        if (s=="") return "";
        for(const char plain : s) {
          if (plain==' ') {
            ret += ' ';
            }
            else {
            const char crypt = '!' + (plain - '!' + 47) % 94;
            ret += crypt;
            }
        }
        return ret;
}

static void send_last_x_lines_of_file(const char* fname, size_t x) {
	std::vector<std::string> lines;

	char buffer[256];

	FILE *fptr = fopen(fname, "r");

	fgets(buffer, 256, fptr);
	while (!feof(fptr)) {
		if (buffer[strlen(buffer) - 1] == '\n') {
			buffer[strlen(buffer) - 1] = '\0';
		}
		if (buffer[strlen(buffer) - 1] == '\r') {
			buffer[strlen(buffer) - 1] = '\0';
		}
		lines.push_back(std::string(buffer));
		fgets(buffer, 256, fptr);
	}
	fclose(fptr);

	size_t start = 0;

	if (lines.size() > x) {
		start = lines.size() - x;
	}

	for (size_t i = start; i < lines.size(); i++) {
		od_disp_emu(lines.at(i).c_str(), true);
		od_printf("\r\n");
	}

	return;
}

static std::vector<std::string> word_wrap(std::string str, int len) {
	std::vector<std::string> strvec;
	size_t line_start = 0;
	size_t last_space = 0;

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == ' ') {
			last_space = i;
		}
		if (i - line_start == len) {
			if (last_space == line_start) {
				strvec.push_back(str.substr(line_start, i - line_start));
				line_start = i;
				last_space = i;
			}
			else {
				strvec.push_back(str.substr(line_start, last_space - line_start));
				line_start = last_space + 1;
				i = line_start;
				last_space = line_start;
			}
		}
	}

	if (line_start < str.size()) {
		strvec.push_back(str.substr(line_start));
	}
	return strvec;
}

Program::Program() {
	lastcallers = new std::vector<struct lastcaller_t*>();
}

int Program::run() {
	INIReader inir("xw5-ilc.ini");

	if (inir.ParseError() != 0) {
		od_printf("Couldn't parse xw5-ilc.ini!\r\n");
		return -1;
	}

	wwiv_path = inir.Get("Main", "WWIV Path", "UNKNOWN");
	system_name = inir.Get("Main", "BBS Name", "A WWIV BBS");
	dat_area = inir.Get("Main", "Data Area", "UNKNOWN");
        display = stoi(inir.Get("Main", "display", "10"));
	bbs_address = inir.Get("Main", "BBS Address","wwivbbs.org");
	dontshow = stoi(inir.Get("Main", "dontshow", "255"));

	if (dat_area == "UNKNOWN") {
		od_printf("`bright red`Data Area must be set in xw5-ilc.ini!\r\n");
		return -1;
	}

	if (wwiv_path == "UNKNOWN") {
		od_printf("`bright red`WWIV Path must be set in xw5-ilc.ini!\r\n");
		return -1;
	}
	std::filesystem::path fspath(wwiv_path);
	const auto& config = new wwiv::sdk::Config(fspath);

	if (!config->Load()) {
		od_printf("`bright red`unable to load config!\r\n");
		return -1;
	}
	wwiv::sdk::Networks networks(*config);

	networks.Load();

        wwiv::sdk::UserManager usermanager(*config);

	wwiv::sdk::Subs subs(config->datadir(), networks.networks());

	if (!subs.Load()) {
		od_printf("Unable to open subs. \r\n");
		return -1;
	}
	if (!subs.exists(dat_area)) {
		od_printf("No such area: %s\r\n", dat_area.c_str());
		return -1;
	}

	const wwiv::sdk::subboard_t sub = find_sub(subs, dat_area).value_or(default_sub(dat_area));
	auto x = new wwiv::sdk::msgapi::NullLastReadImpl();
	wwiv::sdk::msgapi::MessageApiOptions opts;

	const auto& api = std::make_unique<wwiv::sdk::msgapi::WWIVMessageApi>(opts, *config, networks.networks(), x); 
	
	std::unique_ptr<wwiv::sdk::msgapi::MessageArea> area(api->CreateOrOpen(sub, -1));
	
	for (auto current = 1; current <=  area->number_of_messages(); current++) {
		auto message = area->ReadMessage(current);
		if (message->header().title() == "ibbslastcall-data") {
			std::vector<std::string> lines;
			std::stringstream ss(message->text().string());
			std::string tmp;
			while (std::getline(ss, tmp)) {
				if (tmp[0] != 4) {
					lines.push_back(strip_annoying_stuff(tmp));
				}
			}

			if (lines.size()>6) {
				struct lastcaller_t* lcline = new struct lastcaller_t();
				lcline->lastcaller.str("");
				lcline->user = rot47(lines[1]);
				lcline->bbsname = rot47(lines[2]);
	                        lcline->currentdate = rot47(lines[3]);
                                lcline->currenttime = rot47(lines[4]);
	                        lcline->usercity = rot47(lines[5]);
				lcline->systemos = rot47(lines[6]);
				lcline->bbsaddress = rot47(lines[7]);
				lastcallers->push_back(lcline);
			}
		}
	}
        auto lastonfile = wwiv::core::FilePath(networks.networks().at(sub.nets[0].net_num).dir,  LASTON_TXT);
	FILE* fptr = fopen(lastonfile.c_str(), "wb");
	if (!fptr) {
		return -1;
	}
	fprintf(fptr, "|#4 InterBBS Last Callers for: %s  \n",networks.networks().at(sub.nets[0].net_num).name.c_str());
        fprintf(fptr, "|#7\xB3|#2Name/Handle  |#7\xB3|#2 Time |#7\xB3|#2  Date  |#7\xB3|#2 City                   |#7\xB3|#2 BBS                  |#7\xBA\r\n");
        fprintf(fptr, "|#7\xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB6\r\n");
	for (size_t i = lastcallers->size() - display; i < lastcallers->size(); i++) {
            fprintf(fptr,"|#7\xB3|#1%-13.13s",lastcallers->at(i)->user.c_str());
	    fprintf(fptr,"|#7\xB3|#1%-6.6s",lastcallers->at(i)->currenttime.c_str());
	    fprintf(fptr,"|#7\xB3|#1%-8.8s",lastcallers->at(i)->currentdate.c_str());
	    fprintf(fptr,"|#7\xB3|#1%24.24s",lastcallers->at(i)->usercity.c_str());
	    fprintf(fptr,"|#7\xB3|#1%-22.22s",lastcallers->at(i)->bbsname.c_str());
	    fprintf(fptr,"|#7\xBA|#0\r\n");
//            printf("\r\nDEBUG: User: %s - OS: %s\r\n",lastcallers->at(i)->user.c_str(),lastcallers->at(i)->systemos.c_str());
	}
	fprintf(fptr,"|#7\xD4\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");
	fprintf(fptr,"\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF");
	fprintf(fptr,"\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");
	fprintf(fptr,"\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\r\n");
	fclose(fptr);

// Finished reading messages and generating lastcallers files for network

	if (od_control_get()->user_security < dontshow) {

		// load user data given user_num
                // we needed this for name and location :P
                wwiv::sdk::User u{};
                usermanager.readuser(&u ,od_control_get()->user_num);

		// we post to the same board
		std::stringstream ss;
		ss.str("");
		ss << ">>> BEGIN\r\n";
		ss << rot47(u.name()) << "\r\n";
		ss << rot47(system_name) << "\r\n";
                std::time_t time = std::time({});
                char dateString[std::size("mm/dd/yy")];
                char timeString[std::size("00:00pm")];
                std::strftime(std::data(timeString), std::size(timeString),"%I:%M%P", std::localtime(&time));
                std::strftime(std::data(dateString), std::size(dateString),"%m/%d/%y", std::localtime(&time));
                ss << rot47(dateString) << "\r\n";
		ss << rot47(timeString) << "\r\n";
		ss << rot47(u.city()) << "\r\n";
		ss << rot47(wwiv::os::os_version_string().c_str()) << "\r\n";
		ss << rot47(bbs_address) << "\r\n";
		ss << ">>> END\r\n\r\nby [wwiv5lastcaller]\r\n" ;
//		printf("\r\nDEBUG:\r\n%s\r\n", wwiv::os::os_version_string().c_str());
//		fflush(stdout);
		auto msg = area->CreateMessage();
		auto& header = msg.header();
		auto daten = wwiv::core::DateTime::now();
		header.set_from_system(0);
		header.set_from_usernum(0);  // was od_control_get()->user_num);
		header.set_title("ibbslastcall-data");
		header.set_from("ibbslastcall");
		header.set_to("All");
		header.set_daten(daten.to_daten_t());
		msg.set_text(ss.str());

		wwiv::sdk::msgapi::MessageAreaOptions area_options{};

		area_options.send_post_to_network = false;
		std::filesystem::path cpath = std::filesystem::current_path();
		chdir(wwiv_path.c_str());
		area->AddMessage(msg, area_options);
		chdir(cpath.c_str());
	}
	return 0;
}
