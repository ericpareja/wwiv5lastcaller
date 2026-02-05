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
#include <fmt/format.h>
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

std::string Program::create_post_text(wwiv::sdk::UserManager& usermanager) {
	wwiv::sdk::User u{};
	usermanager.readuser(&u, od_control_get()->user_num);

	std::stringstream ss_post;
	ss_post << ">>> BEGIN\r\n";
	if (u.name() == "") {
		ss_post << rot47("Unknown") << "\r\n";
	}
	else {
		ss_post << rot47(u.name()) << "\r\n";
	}
	ss_post << rot47(system_name) << "\r\n";
	std::time_t now_time = std::time(nullptr);
	char dateString[10];
	char timeString[10];
	std::strftime(dateString, sizeof(dateString), "%m/%d/%y", std::localtime(&now_time));
	std::strftime(timeString, sizeof(timeString), "%I:%M%P", std::localtime(&now_time));
	ss_post << rot47(dateString) << "\r\n";
	ss_post << rot47(timeString) << "\r\n";
	if (u.city() == "") {
		ss_post << rot47("Unknown") << "\r\n";
	}
	else {
		ss_post << rot47(u.city()) << "\r\n";
	}
	ss_post << rot47(wwiv::os::os_version_string().c_str()) << "\r\n";
	ss_post << rot47(bbs_address) << "\r\n";
	ss_post << ">>> END\r\n\r\nby [wwiv5lastcaller]\r\n";
	return ss_post.str();
}

int Program::run(int s) {
	INIReader inir("wwiv.ini");

	if (inir.ParseError() != 0) {
		od_printf("Couldn't parse wwiv.ini!\r\n");
		return -1;
	}
	wwiv_path = inir.Get("xw5-ilc", "WWIV Path", "UNKNOWN");
	system_name = inir.Get("xw5-ilc", "BBS Name", "A WWIV BBS");
	dat_area = inir.Get("xw5-ilc", "Data Area", "UNKNOWN");
	display = stoi(inir.Get("xw5-ilc", "display", "10"));
	bbs_address = inir.Get("xw5-ilc", "BBS Address", "wwivbbs.org");
	dontshow = stoi(inir.Get("xw5-ilc", "dontshow", "255"));

	if (dat_area == "UNKNOWN") {
		od_printf("`bright red`Data Area must be set in wwiv.ini!\r\n");
		return -1;
	}

	if (wwiv_path == "UNKNOWN") {
		od_printf("`bright red`WWIV Path must be set in wwiv.ini!\r\n");
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
		printf("Unable to open subs. \r\n");
		return -1;
	}

	std::vector<std::string> areas;
	{
		std::stringstream ss(dat_area);
		std::string segment;
		while (std::getline(ss, segment, ',')) {
			// Trim leading/trailing whitespace
			segment.erase(0, segment.find_first_not_of(" \t\r\n"));
			size_t last = segment.find_last_not_of(" \t\r\n");
			if (last != std::string::npos) {
				segment.erase(last + 1);
			}
			if (!segment.empty()) {
				areas.push_back(segment);
			}
		}
	}

	if (areas.empty()) {
		od_printf("`bright red`No valid Data Areas found in wwiv.ini!\r\n");
		return -1;
	}

	auto x = new wwiv::sdk::msgapi::NullLastReadImpl();
	wwiv::sdk::msgapi::MessageApiOptions opts;
	const auto& api = std::make_unique<wwiv::sdk::msgapi::WWIVMessageApi>(opts, *config, networks.networks(), x);

	for (const auto& area_name : areas) {
		if (!subs.exists(area_name)) {
			od_printf("No such area: %s\r\n", area_name.c_str());
			printf("No such area: %s\r\n", area_name.c_str());
			continue;
		}
		od_printf("\r\n");

		const wwiv::sdk::subboard_t sub = find_sub(subs, area_name).value_or(default_sub(area_name));
		std::unique_ptr<wwiv::sdk::msgapi::MessageArea> area(api->CreateOrOpen(sub, -1));

		if (!area) {
			od_printf("Unable to open area: %s\r\n", area_name.c_str());
			continue;
		}

		for (auto current = 1; current <= area->number_of_messages(); current++) {
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

				if (lines.size() > 6) {
					struct lastcaller_t* lcline = new struct lastcaller_t();
					lcline->lastcaller.str("");
					lcline->user = lines[1] == "" ? "Unknown" : rot47(lines[1]);
					lcline->bbsname = rot47(lines[2]);
					lcline->currentdate = rot47(lines[3]);
					lcline->currenttime = rot47(lines[4]);
					lcline->usercity = lines[5] == "" ? "Unknown" : rot47(lines[5]);
					lcline->systemos = rot47(lines[6]);
					lcline->bbsaddress = rot47(lines[7]);
					lastcallers[area_name].push_back(lcline);
					//					od_printf("`bright white`%s\r", lcline->bbsname.c_str());
				}
			}
		}
	}
        od_printf("`normal`\r\n");
	// Now that we have all callers, generate files and post for each area
	for (const auto& area_name : areas) {
		if (!subs.exists(area_name)) continue;

		const wwiv::sdk::subboard_t sub = find_sub(subs, area_name).value_or(default_sub(area_name));
		std::unique_ptr<wwiv::sdk::msgapi::MessageArea> area(api->CreateOrOpen(sub, -1));
		if (!area) continue;

		if (sub.nets.empty()) continue;

		auto lastonfile = wwiv::core::FilePath(networks.networks().at(sub.nets[0].net_num).dir, LASTON_TXT);
		FILE* fptr = fopen(lastonfile.c_str(), "wb");
		if (!fptr) {
			continue;
		}
		auto networkname = networks.networks().at(sub.nets[0].net_num).name;
		auto title = "InterBBS Last Callers for: " + networkname;
		auto print_table = [&](FILE* out) {
			fmt::print(out, "|#4{:^79}|#0\r\n", title);
			fmt::print(out, "|#7\xB3|#2 Name/Handle |#7\xB3|#2 Time |#7\xB3|#2 Date   |#7\xB3|#2 City                   |#7\xB3|#2 BBS                  |#7\xBA|#0\r\n");
			fmt::print(out, "|#7\xC3\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC5\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xB6|#0\r\n");

			size_t start_idx = (lastcallers[area_name].size() > (size_t)display) ? (lastcallers[area_name].size() - (size_t)display) : 0;
			for (size_t i = start_idx; i < lastcallers[area_name].size(); i++) {
				fmt::print(out, "|#7\xB3|#1{:<13.13}", lastcallers[area_name].at(i)->user);
				fmt::print(out, "|#7\xB3|#1{:<6.6}", lastcallers[area_name].at(i)->currenttime);
				fmt::print(out, "|#7\xB3|#1{:<8.8}", lastcallers[area_name].at(i)->currentdate);
				fmt::print(out, "|#7\xB3|#1{:^24}", lastcallers[area_name].at(i)->usercity);
				fmt::print(out, "|#7\xB3|#1{:<22.22}", lastcallers[area_name].at(i)->bbsname);
				fmt::print(out, "|#7\xBA|#0\r\n");
			}
			fmt::print(out, "|#7\xD4\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");
			fmt::print(out, "\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF");
			fmt::print(out, "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");
			fmt::print(out, "\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC\r\n");
		};

		print_table(fptr);
		fclose(fptr);
		od_printf("`BLUE`xw5-ilc: lastcallers generated for %s.\r\n", networkname.c_str());
		od_printf("`yellow`%s\r\n",sub.nets[0].stype.c_str());
		/*		if (od_control_get()->baud == 0) {
		 *	print_table(stdout);
		 * }
		 */

		// Posting logic for this area
		if (od_control_get()->user_security < dontshow){
			auto post_text = create_post_text(usermanager);
			auto msg = area->CreateMessage();
			auto& header = msg.header();
			auto daten = wwiv::core::DateTime::now();
			header.set_from_system(0);
			header.set_from_usernum(0);
			header.set_title("ibbslastcall-data");
			header.set_from("ibbslastcall");
			header.set_to("All");
			header.set_daten(daten.to_daten_t());
			msg.set_text(post_text);
			wwiv::sdk::msgapi::MessageAreaOptions area_options{};
			area_options.send_post_to_network = true;
			area_options.add_re_and_by_line = false;
			std::filesystem::path cpath = std::filesystem::current_path();
			chdir(wwiv_path.c_str());
			area->AddMessage(msg, area_options);
			od_printf("`white`Posted to: %s\r\n`normal`",area_name.c_str());
			chdir(cpath.c_str());
		}
	}

	od_printf("`GREEN`xw5-ilc: Your SL: %d  DontShow: %d\r\n\r\n", od_control_get()->user_security, dontshow);
	if (od_control_get()->user_security >= dontshow || !od_control_get()->baud == 0) {
		od_printf("`RED`xw5-ilc didn't show you on InterBBS LastCallers.\r\n\r\n");
	}
	else {
		od_printf("`bright yellow`xw5-ilc showed you on InterBBS LastCallers.\r\n\r\n");
	}
	od_printf("`normal`\r\n\r\n\r\n");
	od_printf("\r\n");
	return 0;
}
