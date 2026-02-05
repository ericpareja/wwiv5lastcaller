#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>

namespace wwiv {
namespace sdk {
class UserManager;
}
}

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
	Program() = default;
	int run(int s);
private:
	std::map<std::string, std::vector<struct lastcaller_t *>> lastcallers;
	std::string wwiv_path;
	std::string system_name;
	std::string dat_area;
	int display = 10;
        std::string bbs_address;
        int dontshow = 255;
	std::string create_post_text(wwiv::sdk::UserManager& usermanager);
};

