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

    std::string parse_value(const std::string& value) {
        if (value == "NULL") return "";
        if (value == "\\sp") return " ";
        return value;
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
                std::string val = parse_value(it->second);
                result.replace(pos, end-pos+2, val);
                pos += val.length();
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
            return parse_value(variables[lhs]) == parse_value(rhs);
        }
        if ((op_pos = e.find("!=")) != std::string::npos) {
            std::string lhs = e.substr(0, op_pos);
            std::string rhs = e.substr(op_pos+2);
            return parse_value(variables[lhs]) != parse_value(rhs);
        }
        return parse_value(variables[e]) == "true";
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
            std::string line = script_lines[i];
            if (line.find("//") != std::string::npos) {
                line = line.substr(0, line.find("//"));
            }
            std::istringstream iss(line);
            std::string cmd;
            if (iss >> cmd && cmd == "func") {
                std::string func_name;
                
                skip_mode = true;
                skip_depth = 1;
                
                if (iss >> func_name) {
                    function_locations[func_name] = i;

                }
            }
        }
    }

    void run() {
        preprocess_functions();
        while (current_line < script_lines.size() && !exit_requested) {
            std::string line = script_lines[current_line];
            if (line.find("//") == 0) {
                current_line++;
                continue;
            }
            execute(line);
            current_line++;
        }
    }

private:
    size_t find_matching_end(const std::string& start_cmd, const std::string& end_cmd) {
        int depth = 0;
        for (size_t i = current_line; i < script_lines.size(); ++i) {
            std::string line = script_lines[i];
            if (line.find("//") == 0) continue;
            std::istringstream iss(line);
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
        std::string clean_cmd = command;
        if (clean_cmd.find("//") != std::string::npos) {
            clean_cmd = clean_cmd.substr(0, clean_cmd.find("//"));
        }
        if (skip_mode) {
            if (clean_cmd.find("endif") != std::string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("endfunc") != std::string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("if") == 0 || clean_cmd.find("else") == 0) {
                skip_depth++;
            }
            return;
        }

        std::istringstream iss(clean_cmd);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) return;

        if (cmd == "set") {
            std::string var, value;
            iss >> var;
            getline(iss >> std::ws, value);
            variables[var] = parse_value(parse_string(value));
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
            variables[var] = parse_value(input);
        }
        else if (cmd == "call") {
            std::string func_name;
            iss >> func_name;
            
            if (function_locations.count(func_name)) {
                
                call_stack.push(current_line + 1);
                
                current_line = function_locations[func_name];
                
                size_t end_func_line = -1;
                
                for (size_t i = current_line + 1; i < script_lines.size(); ++i) {
                	std::istringstream iss_end (script_lines[i]);
                	
                	std::string cmd_end;
                	iss_end >> cmd_end;
                	
                	if (cmd_end == "endfunc") {
                		end_func_line = i;

                		break;
                	}
                }
                
                if (end_func_line != -1) {
                	size_t func_start = current_line + 1;
                    while (func_start < end_func_line) {
                   execute(script_lines[func_start]);
                        func_start++;
                    }
                	current_line = call_stack.top();
                    call_stack.pop();
                    
                    
                } else {
                	std::cerr <<"Error: Missing 'endfunc' for function " << func_name << std::endl;
                	return;
                }    
                
            } else {
            	std::cerr << "Error: Function '" << func_name << "' not found." << std::endl;
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
            if (!evaluate_condition(condition)) {
                skip_mode = true;
                skip_depth = 1;
            }
            size_t endif_pos = find_matching_end("if", "endif");
            if_stack.push(endif_pos);
        }
        else if (cmd == "else") {
            skip_mode = true;
            skip_depth = 1;
            current_line = if_stack.top();
            if_stack.pop();
        }
        else if (cmd == "endif") {
            if (!if_stack.empty()) {

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
