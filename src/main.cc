#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "lexer.hh"
#include "parser.hh"
#include "interpreter.hh"

int main(int argc, char** argv) {
    std::vector<std::string> parameters(argv, argv + argc);

    std::ifstream file;
    std::string filename = "<stdin>";
    std::istream* input = &std::cin;
    if (parameters.size() > 1) {
        if (parameters[1] == "-h" || parameters[1] == "--help") {
            std::cout << "Usage: " << parameters[0] << " [-h] [file]\n"
                      << '\n'
                      << "If given a filename, execute program contained in the file.  Othwerise, execute\n"
                      << "standard input terminated by end-of-file.\n"
                      << '\n'
                      << "Options:\n"
                      << "\t-h, --help\tPrint this text and exit\n";
            return 0;
        } else {
            filename = parameters[1];
            file.open(filename.c_str());
            if (file)
                input = &file;
            else {
                std::cerr << "Can't open " << parameters[1] << " for reading\n";
                return 1;
            }
        }
    }

    try {
        lexer lexer(*input, filename);
        block program = parse(lexer);
        interpreter interpreter(program);
        interpreter.run();

        std::cout << std::flush;

    } catch (lexer_error const& error) {
        std::cerr << "Lexer error: " << error.what() << '\n';
    } catch (syntax_error const& error) {
        std::cerr << "Syntax error: " << error.what() << '\n';
    } catch (runtime_error const& error) {
        std::cerr << "Runtime error: " << error.what() << '\n';
    } catch (std::exception const& error) {
        std::cerr << "Internal error: " << error.what() << '\n';
    }
}
