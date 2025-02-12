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
    std::stack<size_t> loop_stack;
    std::stack<size_t> if_stack;
    size_t current_line = 0;
    bool exit_requested = false;
    bool skip_mode = false;
    int skip_depth = 0;
    bool break_requested = false;

    int safe_stoi(const std::string& s) {
        if (s.empty()) throw std::invalid_argument("Empty string");
        for (char c : s) if (!isdigit(c)) throw std::invalid_argument("Invalid number");
        return std::stoi(s);
    }

    std::string parse_string(const std::string& text) {
        std::string result = text;
        size_t pos = 0;
        while ((pos = result.find("{{", pos)) != std::string::npos) {
            size_t end = result.find("}}", pos);
            if (end == std::string::npos) break;
            
            std::string var = result.substr(pos+2, end-pos-2);
            var.erase(0, var.find_first_not_of(" \t"));
            var.erase(var.find_last_not_of(" \t") + 1);
            
            auto it = variables.find(var);
            if (it != variables.end()) {
                result.replace(pos, end-pos+2, it->second);
                pos += it->second.length();
            } else {
                result.erase(pos, end-pos+2);
            }
        }
        return result;
    }

    bool evaluate_condition(const std::string& expr) {
        std::string e = parse_string(expr);
        size_t op_pos;
        
        if ((op_pos = e.find("==")) != std::string::npos) {
            std::string lhs = e.substr(0, op_pos);
            std::string rhs = e.substr(op_pos+2);
            return variables[lhs] == rhs;
        }
        if ((op_pos = e.find("!=")) != std::string::npos) {
            std::string lhs = e.substr(0, op_pos);
            std::string rhs = e.substr(op_pos+2);
            return variables[lhs] != rhs;
        }
        return variables[e] == "true";
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
    size_t find_matching_end(const std::string& start_cmd, const std::string& end_cmd) {
        int depth = 1;
        for (size_t i = current_line + 1; i < script_lines.size(); ++i) {
            std::istringstream iss(script_lines[i]);
            std::string cmd;
            if (iss >> cmd) {
                if (cmd == start_cmd) depth++;
                else if (cmd == end_cmd) depth--;
                
                if (depth == 0) return i;
            }
        }
        throw std::runtime_error("Unclosed " + start_cmd);
    }

    void execute(const std::string& command) {
        if (break_requested) return;
        
        if (skip_mode) {
            if (command.find("endif") != std::string::npos) {
                skip_depth--;
                if (skip_depth == 0) skip_mode = false;
            }
            else if (command.find("if") != std::string::npos) {
                skip_depth++;
            }
            return;
        }

        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "set") {
            std::string var, value;
            iss >> var;
            getline(iss >> std::ws, value);
            variables[var] = parse_string(value);
        }
        else if (cmd == "print") {
            std::string text;
            getline(iss >> std::ws, text);
            std::cout << parse_string(text) << std::endl;
        }
        else if (cmd == "input") {
            std::string var;
            iss >> var;
            std::string input;
            std::getline(std::cin, input);
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
            loop_stack.push(current_line);
        }
        else if (cmd == "endloop") {
            if (!loop_stack.empty()) {
                current_line = loop_stack.top() - 1;
                loop_stack.pop();
            }
        }
        else if (cmd == "if") {
            std::string condition;
            getline(iss >> std::ws, condition);
            bool result = evaluate_condition(condition);
            
            if (!result) {
                skip_mode = true;
                skip_depth = 1;
                if_stack.push(find_matching_end("if", "endif"));
            }
            else {
                if_stack.push(find_matching_end("if", "endif"));
            }
        }
        else if (cmd == "endif") {
            if (!if_stack.empty()) {
                current_line = if_stack.top();
                if_stack.pop();
            }
        }
        else if (cmd == "break") {
            if (!loop_stack.empty()) {
                size_t loop_end = find_matching_end("loop", "endloop");
                current_line = loop_end;
                break_requested = true;
            }
        }
        else if (cmd == "exit") {
            exit_requested = true;
        }
    }
};
