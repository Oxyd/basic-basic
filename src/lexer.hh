#ifndef LEXER_HH
#define LEXER_HH

#include <string>
#include <vector>
#include <istream>
#include <stdexcept>

#include <boost/optional.hpp>

struct lexeme {
    enum e_type {
        type_word,
        type_symbol,
        type_number,
        type_string,
        type_end        // End of statement (logical line).
    };

    struct physical_location_t {
        std::string filename;
        int line;
        int column;
    } physical_location;

    e_type      type;
    std::string value;

    lexeme(e_type type, std::string const& value, std::string const& filename, int line, int column);
};

struct lexer_error : std::runtime_error {
    explicit lexer_error(std::string const& what);
};

class lexer {
public:
    explicit lexer(std::istream& source, std::string const& filename = "<input>");

    // Return the next lexeme that is in the lexer.  If end of the stream has been encountered, return none.
    boost::optional<lexeme> get_lexeme();

    // Discard all input until the end of the line.
    void ignore_line();

private:
    std::istream& source;

    void skip_whitespace();

    // Read characters from source until 'what' has been seen, and return the read portion.  'What' is not
    // extracted from the stream and it is not a part of the return value.
    std::string extract_until(char what);

    // Same as above, but stop on any character for which pred(char) == true.
    template <typename UnaryFunction>
    std::string extract_until(UnaryFunction pred);

    std::string const filename;
    int line;
    int column;
};

#endif
