#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <stack>
#include <stdexcept>
#include <cctype>
#include <algorithm>

class KiwiInterpreter {
private:
    std::map<std::string, std::string> variables;
    std::vector<std::string> script_lines;
    std::map<std::string, size_t> function_locations;
    std::stack<size_t> call_stack;
    size_t current_line = 0;
    bool exit_requested = false;
    std::stack<size_t> loop_ends;

    int safe_stoi(const std::string& s) {
        if (s.empty()) throw std::invalid_argument("Empty string");
        for (char c : s) {
            if (!isdigit(c) throw std::invalid_argument("Invalid number");
        }
        return std::stoi(s);
    }

    std::string get_var_value(const std::string& var_name) {
        auto it = variables.find(var_name);
        if (it != variables.end()) return it->second;
        throw std::runtime_error("Undefined variable: " + var_name);
    }

    std::string parse_string(const std::string& text) {
        std::string result;
        size_t pos = 0;
        while (pos < text.size()) {
            if (text[pos] == '{' && pos + 1 < text.size() && text[pos+1] == '{') {
                pos += 2;
                std::string var_name;
                while (pos < text.size() && text[pos] != '}') {
                    var_name += text[pos++];
                }
                if (pos < text.size()) pos++;
                var_name.erase(0, var_name.find_first_not_of(" \t"));
                var_name.erase(var_name.find_last_not_of(" \t") + 1);
                result += get_var_value(var_name);
            } else {
                result += text[pos++];
            }
        }
        return result;
    }

public:
    void load_script(const std::vector<std::string>& lines) {
        script_lines = lines;
        current_line = 0;
        exit_requested = false;
        variables.clear();
        function_locations.clear();
    }

    void preprocess_functions() {
        for (size_t i = 0; i < script_lines.size(); ++i) {
            std::istringstream iss(script_lines[i]);
            std::string cmd;
            if (iss >> cmd && cmd == "func") {
                std::string func_name;
                if (iss >> func_name) {
                    function_locations[func_name] = i;
                }
            }
        }
    }

    void run() {
        preprocess_functions();
        while (current_line < script_lines.size() && !exit_requested) {
            execute(script_lines[current_line]);
            current_line++;
        }
    }

private:
    void execute(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "set") {
            std::string var, value;
            iss >> var;
            std::getline(iss >> std::ws, value);
            variables[var] = parse_string(value);
        }
        else if (cmd == "print") {
            std::string text;
            std::getline(iss >> std::ws, text);
            std::cout << parse_string(text) << std::endl;
        }
        else if (cmd == "input") {
            std::string var;
            iss >> var;
            std::string input;
            std::getline(std::cin >> std::ws, input);
            variables[var] = input;
        }
        else if (cmd == "call") {
            std::string func_name;
            iss >> func_name;
            if (function_locations.count(func_name)) {
                call_stack.push(current_line);
                current_line = function_locations[func_name];
            }
        }
        else if (cmd == "loop") {
            loop_ends.push(skip_to_end(current_line, "endloop"));
        }
        else if (cmd == "endloop") {
            if (!loop_ends.empty()) {
                current_line = loop_ends.top() - 1;
                loop_ends.pop();
            }
        }
        else if (cmd == "exit") {
            exit_requested = true;
        }
        // Добавьте обработку других команд по аналогии
    }

    size_t skip_to_end(size_t start, const std::string& end_cmd) {
        for (size_t i = start + 1; i < script_lines.size(); ++i) {
            std::istringstream iss(script_lines[i]);
            std::string cmd;
            if (iss >> cmd && cmd == end_cmd) {
                return i;
            }
        }
        throw std::runtime_error("Missing " + end_cmd);
    }
};
