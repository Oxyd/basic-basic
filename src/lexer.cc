#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>
#include <istream>
#include <sstream>
#include <cctype>

#include <iostream>

#include <boost/optional.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "lexer.hh"

namespace {

char const          WHITESPACE[] = { ' ', '\t' };
char const* const   WHITESPACE_END = WHITESPACE + sizeof(WHITESPACE);

// Symbols that can be lexed without seeing what follows them.
char const          SIMPLE_SYMBOLS[] = { '+', '-', '*', '/', '&', '=', ':', ',', '(', ')' };
char const* const   SIMPLE_SYMBOLS_END = SIMPLE_SYMBOLS + sizeof(SIMPLE_SYMBOLS);

// Helpers.
std::pointer_to_unary_function<int, int> is_digit = std::ptr_fun(&isdigit);

// Basically the same as C's isalnum, but also accepts _ and $ as a valid character.
int isalphanum(int c) {
    return isalnum(c) || c == '_' || c == '$';
}

std::pointer_to_unary_function<int, int> is_alphanum = std::ptr_fun(&isalphanum);

std::unary_negate<std::pointer_to_unary_function<int, int> > is_not_digit = std::not1(is_digit);
std::unary_negate<std::pointer_to_unary_function<int, int> > is_not_alnum = std::not1(is_alphanum);

}

lexeme::lexeme(e_type type, std::string const& value, std::string const& filename, int line, int column)
    : type(type)
    , value(value)
{
    physical_location.filename = filename;
    physical_location.line = line;
    physical_location.column = column;

    if (type == lexeme::type_word)
        boost::algorithm::to_lower(this->value);
    else if (type == lexeme::type_number) {
        // Remove leading zeroes from integers.
        if (value[0] == '0' && value.length() > 1) {
            std::string::size_type first_nonzero = value.find_first_not_of('0');
            if (first_nonzero != value.npos)
                this->value = value.substr(first_nonzero);
        }
    }
}

lexer_error::lexer_error(std::string const& what)
    : std::runtime_error(what)
{ }

lexer::lexer(std::istream& source, std::string const& filename)
    : source(source)
    , filename(filename)
    , line(1)
    , column(0)
{ }

boost::optional<lexeme> lexer::get_lexeme() {
    skip_whitespace();

    char next_char;
    if (source && (next_char = source.peek()) != std::istream::traits_type::eof()) {
        if (isdigit(next_char)) {
            std::string const whole_part = extract_until(is_not_digit);
            if (source.peek() == '.') {
                // This is a decimal number.
                source.get();
                ++column;
                std::string const decimal_part = extract_until(is_not_digit);
                return lexeme(lexeme::type_number, whole_part + '.' + decimal_part, filename, line, column);
            } else {
                return lexeme(lexeme::type_number, whole_part, filename, line, column);
            }

        } else if (next_char == '"') {
            source.get();  // Consume the starting ".
            ++column;
            std::string const value = extract_until('"');
            lexeme result(lexeme::type_string, value, filename, line, column);

            source.get();  // Consume the trailing ".
            ++column;
            return result;

        } else if (std::find(SIMPLE_SYMBOLS, SIMPLE_SYMBOLS_END, next_char) != SIMPLE_SYMBOLS_END) {
            // Next character is a simple symbol.
            return lexeme(lexeme::type_symbol, std::string(1, source.get()), filename, line, ++column);

        } else if ((next_char == '<' || next_char == '>')) {
            source.get();  // Discard the current one.
            ++column;

            if (source.peek() == '>' || source.peek() == '=') {
                // We've got a two-character lexeme.
                char const second_char = source.get();
                ++column;

                if (second_char == '=' || (next_char == '<' && second_char == '>'))
                    return lexeme(lexeme::type_symbol, std::string() + next_char + second_char, filename, line, column);
                else
                    throw lexer_error(std::string("Invalid operator: ") + next_char + second_char);
            } else {
                // A single-charactrer lexeme.
                return lexeme(lexeme::type_symbol, std::string(1, next_char), filename, line, column);
            }

        } else if (next_char == '\n') {
            // Discard any consecutive blank lines.
            while (source.peek() == '\n') {
                source.get();
                ++line;
                column = 0;
                skip_whitespace();
            }

            return lexeme(lexeme::type_end, "", filename, line, column);

        } else if (isalpha(next_char)) {
            std::string const value = extract_until(is_not_alnum);
            return lexeme(lexeme::type_word, value, filename, line, column);

        } else {
            std::ostringstream os;
            os << "Invalid character at input: '" << static_cast<char>(source.peek())
               << "' (" << static_cast<int> (source.peek()) << ")";
            throw lexer_error(os.str());
        }
    }

    return boost::optional<lexeme>();
}

void lexer::ignore_line() {
    while (source && source.peek() != '\n') {
        source.get();
        ++column;
    }
}

void lexer::skip_whitespace() {
    while (source) {
        char next = source.peek();
        if (std::find(WHITESPACE, WHITESPACE_END, next) == WHITESPACE_END)
            break;
        else {
            source.get();
            ++column;
        }
    }
}

std::string lexer::extract_until(char what) {
    return extract_until(std::bind2nd(std::equal_to<char>(), what));
}

template <typename UnaryFunction>
std::string lexer::extract_until(UnaryFunction pred) {
    std::string result;

    while (source && !pred(source.peek())) {
        result += source.get();
        ++column;
    }

    return result;
}
