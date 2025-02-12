#include "interpreter.hpp"
#include <fstream>

int main() {
    KiwiInterpreter interpreter;
    std::ifstream file("script.kiwi");
    std::vector<std::string> lines;
    std::string line;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    interpreter.load_script(lines);
    interpreter.run();
    
    return 0;
}
