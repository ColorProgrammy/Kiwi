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
#include <ctime>
#include <cmath>
#include <variant>
#include <chrono>
#include <iomanip>

using namespace std;

class KiwiInterpreter {
private:
    // Value type can be either string or double
    using Value = variant<string, double>;
    
    // Execution state components
    map<string, Value> variables;           // Stores all variables
    vector<string> script_lines;            // Loaded script lines
    map<string, size_t> function_locations; // Function definitions
    stack<size_t> call_stack;               // Function call stack
    stack<size_t> loop_stack;               // Loop control stack
    stack<size_t> if_stack;                 // Conditional stack
    size_t current_line = 0;                // Current line pointer
    bool exit_requested = false;            // Exit flag
    bool skip_mode = false;                 // Conditional skip mode
    int skip_depth = 0;                     // Skip nesting depth
    bool break_requested = false;           // Loop break flag

    // Trim whitespace from string
    string trim(const string& s) {
        auto start = s.find_first_not_of(" \t");
        if (start == string::npos) return "";
        auto end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    // Try parsing number from string
    bool try_parse_number(const string& s, double& num) {
        try {
            size_t pos = 0;
            num = stod(s, &pos);
            return pos == s.size();
        } catch (...) {
            return false;
        }
    }

    // Convert stored value to string
    string parse_value(const Value& value) {
        if (holds_alternative<string>(value)) {
            string s = get<string>(value);
            if (s == "NULL") return "";
            if (s == "\\sp") return " ";
            return s;
        }
        return to_string(get<double>(value));
    }

    // Process string with variables and math
    string parse_string(const string& text) {
        string result = text;
        size_t pos = 0;
        
        // Process math expressions in <>
        while ((pos = result.find('<', pos)) != string::npos) {
            size_t end = result.find('>', pos);
            if (end == string::npos) break;
            
            try {
                string expr = result.substr(pos+1, end-pos-1);
                double val = evaluate_math_expression(expr);
                string num_str = to_string(val);
                num_str.erase(num_str.find_last_not_of('0') + 1, string::npos);
                if (num_str.back() == '.') num_str.pop_back();
                result.replace(pos, end-pos+1, num_str);
                pos += num_str.length();
            } catch (...) {
                pos = end + 1;
            }
        }
        
        // Process variables in {{
        while ((pos = result.find("{{", pos)) != string::npos) {
            size_t end = result.find("}}", pos);
            if (end == string::npos) break;
            string var = trim(result.substr(pos+2, end-pos-2));
            if (variables.count(var)) {
                string val = parse_value(variables[var]);
                result.replace(pos, end-pos+2, val);
                pos += val.length();
            } else {
                result.erase(pos, end-pos+2);
            }
        }
        return result;
    }

    // Mathematical expression evaluator
    double evaluate_math_expression(const string& expr) {
        string expression = parse_string(expr);
        stack<double> values;
        stack<char> ops;

        auto apply_op = [&]() {
            double r = values.top(); values.pop();
            double l = values.top(); values.pop();
            char op = ops.top(); ops.pop();
            switch(op) {
                case '+': values.push(l + r); break;
                case '-': values.push(l - r); break;
                case '*': values.push(l * r); break;
                case '/': 
                    if (r == 0) throw runtime_error("Division by zero");
                    values.push(l / r); 
                    break;
                case '^': values.push(pow(l, r)); break;
            }
        };

        auto precedence = [](char op) {
            if (op == '+' || op == '-') return 1;
            if (op == '*' || op == '/') return 2;
            if (op == '^') return 3;
            return 0;
        };

        string token;
        for (size_t i = 0; i < expression.size(); ++i) {
            char c = expression[i];
            
            if (isdigit(c) || c == '.') {
                token += c;
                while (i+1 < expression.size() && 
                      (isdigit(expression[i+1]) || expression[i+1] == '.')) {
                    token += expression[++i];
                }
                values.push(stod(token));
                token.clear();
            }
            else if (isalpha(c) || c == '_') {
                token += c;
                while (i+1 < expression.size() && 
                      (isalnum(expression[i+1]) || expression[i+1] == '_')) {
                    token += expression[++i];
                }
                if (variables.count(token)) {
                    if (holds_alternative<double>(variables[token])) {
                        values.push(get<double>(variables[token]));
                    } else {
                        values.push(stod(get<string>(variables[token])));
                    }
                } else {
                    throw runtime_error("Undefined variable: " + token);
                }
                token.clear();
            }
            else if (c == '(') {
                ops.push(c);
            }
            else if (c == ')') {
                while (!ops.empty() && ops.top() != '(') {
                    apply_op();
                }
                ops.pop();
            }
            else if (isspace(c)) {
                continue;
            }
            else {
                while (!ops.empty() && precedence(ops.top()) >= precedence(c)) {
                    apply_op();
                }
                ops.push(c);
            }
        }

        while (!ops.empty()) {
            apply_op();
        }

        return values.top();
    }

public:
    KiwiInterpreter() {
        srand(time(nullptr)); // Initialize random generator
    }

    // Load script into memory
    void load_script(const vector<string>& lines) {
        script_lines = lines;
        current_line = 0;
        exit_requested = false;
        variables.clear();
        function_locations.clear();
    }

    // Preprocess function definitions
    void preprocess_functions() {
        for (size_t i = 0; i < script_lines.size(); ++i) {
            string line = script_lines[i];
            size_t comment_pos = line.find("//");
            if (comment_pos != string::npos) {
                line = line.substr(0, comment_pos);
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

    // Main execution loop
    void run() {
        preprocess_functions();
        while (current_line < script_lines.size() && !exit_requested) {
            string line = script_lines[current_line];
            if (line.find("//") == 0) { // Skip comments
                current_line++;
                continue;
            }
            execute(line);
            current_line++;
        }
    }

private:
    // Find matching control structure end
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

    // Execute single command
    void execute(const string& command) {
        if (break_requested) return;
        string clean_cmd = command;
        size_t comment_pos = clean_cmd.find("//");
        if (comment_pos != string::npos) {
            clean_cmd = clean_cmd.substr(0, comment_pos);
        }
        
        // Skip mode handling
        if (skip_mode) {
            if (clean_cmd.find("endif") != string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("endfunc") != string::npos) {
                if (--skip_depth == 0) skip_mode = false;
            }
            else if (clean_cmd.find("if") == 0 || 
                    clean_cmd.find("else") == 0) {
                skip_depth++;
            }
            return;
        }

        istringstream iss(clean_cmd);
        string cmd;
        iss >> cmd;
        if (cmd.empty()) return;

        // Command dispatch
        if (cmd == "set") {
            string var, value;
            iss >> var;
            getline(iss >> ws, value);
            try {
                variables[var] = stod(value);
            } catch (...) {
                variables[var] = parse_string(value);
            }
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
            variables[var] = input;
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
        else if (cmd == "math") {
            string var, eq;
            iss >> var >> eq;
            if (eq != "=") {
                cerr << "Invalid math syntax" << endl;
                return;
            }
            string expr;
            getline(iss >> ws, expr);
            try {
                variables[var] = evaluate_math_expression(expr);
            } catch (const exception& e) {
                cerr << "Math error: " << e.what() << endl;
            }
        }
        else if (cmd == "random") {
            string var;
            double min_val, max_val;
            if (iss >> var >> min_val >> max_val) {
                double range = max_val - min_val;
                variables[var] = min_val + fmod(rand(), range + 1);
            }
        }
        else if (cmd == "length") {
            string var, source;
            if (iss >> var >> source) {
                string str = parse_string(source);
                variables[var] = static_cast<double>(str.length());
            }
        }
        else if (cmd == "clear") {
            cout << "\033[2J\033[1;1H"; // ANSI clear screen
        }
        else if (cmd == "time") {
            string var;
            if (iss >> var) {
                auto now = chrono::system_clock::now();
                time_t now_time = chrono::system_clock::to_time_t(now);
                tm local_tm;
                localtime_r(&now_time, &local_tm);
                ostringstream oss;
                oss << put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
                variables[var] = oss.str();
            }
        }
        else if (cmd == "timestamp") {
            string var;
            if (iss >> var) {
                auto now = chrono::system_clock::now();
                auto duration = now.time_since_epoch();
                double seconds = chrono::duration_cast<chrono::seconds>(duration).count();
                variables[var] = seconds;
            }
        }
        else if (cmd == "sleep") {
            double seconds;
            if (iss >> seconds) {
                this_thread::sleep_for(chrono::milliseconds(static_cast<int>(seconds * 1000)));
            }
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
