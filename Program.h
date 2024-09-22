#pragma once

#include <string>
#include <sstream>
#include <vector>

struct lastcaller_t {
	std::stringstream lastcaller;
	std::string user;
	std::string bbsname;
	std::string currentdate;
	std::string currenttime;
	std::string usercity;
	std::string systemos;
        std::string bbsaddress;
};

class Program
{
public:
	Program();
	int run();
private:
	std::vector<struct lastcaller_t *> *lastcallers;
	std::string wwiv_path;
	std::string system_name;
	std::string dat_area;
	int display;
//	std::vector<std::string> add_oneliner();
};

