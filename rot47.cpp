#include <iostream>
#include <string>

char rot47(char c) {
    if (c >= 33 && c <= 126) {
        return (c - 33 + 47) % 94 + 33;
    }
    return c;
}

int main() {
    std::string input;
    while (std::getline(std::cin, input)) {
        for (char& c : input) {
            c = rot47(c);
        }
        std::cout << input << std::endl;
    }
    return 0;
}
