# Kiwi

**A concise and easy-to-learn programming language designed for simple tasks.**

----

Kiwi is a lightweight and intuitive scripting language focused on providing a streamlined experience for automating common tasks and creating small-scale applications.  Its syntax is designed for readability and ease of use, allowing developers to quickly write and execute code with minimal overhead.

**Key Features:**

*   **Simple and Intuitive Syntax:** Kiwi emphasizes clarity and reduces boilerplate, making it easy for beginners to pick up and use.
*   **Concise and Expressive:** Write more with less code, focusing on the essential logic of your programs.
*   **Embeddable:** Kiwi is designed to be easily embedded into C++ projects, extending their functionality with a dynamic scripting layer.
*   **C++11 and later compatibility:** Kiwi is implemented in C++11 and later, ensuring compatibility with modern C++ compilers and features. This allows you to leverage the power of C++ alongside the simplicity of Kiwi.

**License:**

Kiwi is released under the `Apache-2.0 License`. See the `LICENSE` file for the full license text.

**Installation & Usage:**

1.  **Download:** Download the repository as a ZIP file or clone it using Git.
2.  **Include:** To use Kiwi in your C++ project, simply copy the `kiwi` directory into your project's source directory and include the main interpreter header file:

    ```c++
    #include "kiwi/interpreter.hpp"
    ```

3.  **Integrate:**  Create an instance of the `KiwiInterpreter` class in your C++ code and load your Kiwi scripts:

    ```c++
    #include "kiwi/interpreter.hpp"
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
    ```

    (See the examples in the repository for more detailed usage.)

**More Information:**

*   Explore the examples provided in the repository to learn more about the capabilities of Kiwi.
*   Refer to the documentation for a comprehensive guide to the language syntax and features.
*   Contribute to the development of Kiwi by submitting bug reports, feature requests, or pull requests.


**Copyright (c) 2025, ColorProgrammy**
