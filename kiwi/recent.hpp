#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <stack>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <string>
#include <cstdlib>

using namespace std;

class KiwiInterpreter {
private:
    map<string, string> variables;
    vector<string> script_lines;
    map<string, size_t> function_locations;
    stack<size_t> call_stack;
    stack<size_t> loop_stack;
    stack<size_t> if_stack;
    size_t current_line = 0;
    bool exit_requested = false;
    bool skip_mode = false;
    int skip_depth = 0;
    bool break_requested = false;

    // Вспомогательные функции
    string trim(const string& s) {
        size_t start = s.find_first_not_of(" \t");
        if (start == string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    bool try_parse_number(const string& s, double& num) {
        try {
            size_t pos = 0;
            num = stod(s, &pos);
            return pos == s.size();
        } catch (...) {
            return false;
        }
    }

    int safe_stoi(const string& s) {
        if (s.empty()) throw invalid_argument("Empty string");
        for (char c : s) if (!isdigit(c)) throw invalid_argument("Invalid number");
        return stoi(s);
    }

    string parse_value(const string& value) {
        if (value == "NULL") return "";
        if (value == "\\sp") return " ";
        return value;
    }

    string parse_string(const string& text) {
        string result = text;
        size_t pos = 0;
        while ((pos = result.find("{{", pos)) != string::npos) {
            size_t end = result.find("}}", pos);
            if (end == string::npos) break;
            string var = result.substr(pos+2, end-pos-2);
            var = trim(var);
            auto it = variables.find(var);
            if (it != variables.end()) {
                string val = parse_value(it->second);
                result.replace(pos, end-pos+2, val);
                pos += val.length();
            } else {
                result.erase(pos, end-pos+2);
            }
        }
        return result;
    }

    bool evaluate_condition(const string& expr) {
        string e = parse_string(expr);
        size_t op_pos;
        string lhs, rhs;
        double lhs_num, rhs_num;
        bool lhs_is_num, rhs_is_num;
        string lhs_str, rhs_str;

        vector<pair<string, int>> operators = {
            {">=", 2}, {"<=", 2}, {"==", 2}, {"!=", 2},
            {"contains", 8}, {"startswith", 10}, {"endswith", 8},
            {">", 1}, {"<", 1}, {" in ", 4}
        };

        for (auto& op : operators) {
            string op_str = op.first;
            int op_len = op.second;
            if ((op_pos = e.find(op_str)) != string::npos && 
                (op_str != " in " || (op_pos > 0 && e[op_pos-1] == ' ' && e[op_pos+op_len] == ' '))) 
            {
                lhs = trim(e.substr(0, op_pos));
                rhs = trim(e.substr(op_pos + op_len));
                lhs_str = parse_value(variables[lhs]);
                rhs_str = parse_value(rhs);

                // Числовые сравнения
                if (op_str == ">=" || op_str == "<=" || op_str == ">" || op_str == "<") {
                    if (try_parse_number(lhs_str, lhs_num) && try_parse_number(rhs_str, rhs_num)) {
                        if (op_str == ">=") return lhs_num >= rhs_num;
                        if (op_str == "<=") return lhs_num <= rhs_num;
                        if (op_str == ">") return lhs_num > rhs_num;
                        if (op_str == "<") return lhs_num < rhs_num;
                    }
                    return false;
                }
                // Равенства/неравенства
                else if (op_str == "==" || op_str == "!=") {
                    bool num_comp = try_parse_number(lhs_str, lhs_num) && try_parse_number(rhs_str, rhs_num);
                    if (num_comp) {
                        return op_str == "==" ? lhs_num == rhs_num : lhs_num != rhs_num;
                    }
                    return op_str == "==" ? lhs_str == rhs_str : lhs_str != rhs_str;
                }
                // Строковые операции
                else if (op_str == "contains") {
                    return lhs_str.find(rhs_str) != string::npos;
                }
                else if (op_str == "startswith") {
                    return lhs_str.find(rhs_str) == 0;
                }
                else if (op_str == "endswith") {
                    return lhs_str.size() >= rhs_str.size() && 
                        lhs_str.substr(lhs_str.size() - rhs_str.size()) == rhs_str;
                }
                // Проверка вхождения
                else if (op_str == " in ") {
                    vector<string> items;
                    stringstream ss(rhs_str);
                    string item;
                    while (getline(ss, item, ',')) {
                        items.push_back(trim(item));
                    }
                    return find(items.begin(), items.end(), lhs_str) != items.end();
                }
            }
        }

        // Проверка булевого значения по умолчанию
        return parse_value(variables[trim(e)]) == "true";
    }

public:
    void load_script(const vector<string>& lines) {
        script_lines = lines;
        current_line = 0;
        exit_requested = false;
        variables.clear();
        function_locations.clear();
    }

    void preprocess_functions() {
        for (size_t i = 0; i < script_lines.size(); ++i) {
            string line = script_lines[i];
            if (line.find("//") != string::npos) {
                line = line.substr(0, line.find("//"));
            }
            istringstream iss(line);
            string cmd;
            if (iss >> cmd && cmd == "func") {
                string func_name;
                if (iss >> func_name) {
                    function_locations[func_name] = i;
                }
            }
        }
    }

    void run() {
        preprocess_functions();
        while (current_line < script_lines.size() && !exit_requested) {
            string line = script_lines[current_line];
            if (line.find("//") == 0) {
                current_line++;
                continue;
            }
            execute(line);
            current_line++;
        }
    }

private:
    size_t find_matching_end(const string& start_cmd, const string& end_cmd) {
        int depth = 0;
        for (size_t i = current_line; i < script_lines.size(); ++i) {
            string line = script_lines[i];
            if (line.find("//") == 0) continue;
            istringstream iss(line);
            string cmd;
            if (iss >> cmd) {
                if (cmd == start_cmd) depth++;
                else if (cmd == end_cmd) depth--;
                if (depth == 0) return i;
            }
        }
        throw runtime_error("Unclosed " + start_cmd);
    }

    void execute(const string& command) {
        if (break_requested) return;
        string clean_cmd = command;
        if (clean_cmd.find("//") != string::npos) {
            clean_cmd = clean_cmd.substr(0, clean_cmd.find("//"));
        }
        
        if (skip_mode) {
            if (clean_cmd.find("endif") != string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("endfunc") != string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("if") == 0 || clean_cmd.find("else") == 0) {
                skip_depth++;
            }
            return;
        }

        istringstream iss(clean_cmd);
        string cmd;
        iss >> cmd;
        if (cmd.empty()) return;

        if (cmd == "set") {
            string var, value;
            iss >> var;
            getline(iss >> ws, value);
            variables[var] = parse_value(parse_string(value));
        }
        else if (cmd == "print") {
            string text;
            getline(iss >> ws, text);
            cout << parse_string(text) << endl;
        }
        else if (cmd == "input") {
            string var;
            iss >> var;
            string input;
            getline(cin, input);
            variables[var] = parse_value(input);
        }
        else if (cmd == "call") {
            string func_name;
            iss >> func_name;
            if (function_locations.count(func_name)) {
                call_stack.push(current_line + 1);
                current_line = function_locations[func_name];
                size_t end_func_line = find_matching_end("func", "endfunc");
                current_line = end_func_line;
                current_line = call_stack.top();
                call_stack.pop();
            } else {
                cerr << "Error: Function '" << func_name << "' not found." << endl;
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
            string condition;
            getline(iss >> ws, condition);
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
        else if (cmd == "func") {
            skip_mode = true;
            skip_depth = 1;
        }
        else if (cmd == "endfunc") {
            if (!call_stack.empty()) {
                current_line = call_stack.top();
                call_stack.pop();
            }
        }
    }
};
