#include <string>
#include <sstream>
#include <map>
#include <cassert>
#include <memory>

#include <iostream>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "lexer.hh"
#include "statements.hh"
#include "number.hh"
#include "parser.hh"

namespace {

struct line {
    std::string label;
    std::auto_ptr< ::statement > statement;

    line& copy() { return *this; }  // Hack around auto_ptr's uglyness by adding more uglyness.
};

// Keywords that terminate a block.
std::string const BLOCK_TERMINATORS[] = { "end", "else", "elseif", "next", "loop" };
std::string const* const BLOCK_TERMINATORS_END
    = BLOCK_TERMINATORS + sizeof(BLOCK_TERMINATORS) / sizeof(*BLOCK_TERMINATORS);

// Function type that's supposed to extract a whole statement from the parser.
typedef boost::function<std::auto_ptr< ::statement >(lexer&)> statement_parser_t;

// Mapping of keywords to their respective parsers.
typedef std::map<std::string, statement_parser_t> parsers_map_t;

parsers_map_t parsers_map;
boost::optional<lexeme> next_symbol;

std::string to_string(lexeme::e_type type) {
    switch (type) {
    case lexeme::type_end:          return "end of line";
    case lexeme::type_number:      return "integral literal";
    case lexeme::type_string:       return "string literal";
    case lexeme::type_symbol:       return "operator";
    case lexeme::type_word:         return "identifier or keyword";
    }
    assert(!"Never gets here");
    return "";  // Stupid compiler.
}

// Unconditionally accept what's in the stream.
boost::optional<lexeme> accept(lexer& lexer) {
    boost::optional<lexeme> result = next_symbol;
    next_symbol = lexer.get_lexeme();
    return result;
}

// If the next symbol in the lexer is of the given type (and, optionally, contains the given value), return the symbol
// and read the next one.
boost::optional<lexeme> accept(
    lexer& lexer,
    lexeme::e_type what,
    boost::optional<std::string> value = boost::optional<std::string>()
) {

    if (next_symbol && next_symbol->type == what && (!value || next_symbol->value == value))
        return accept(lexer);
    else
        return boost::optional<lexeme>();
}

syntax_error error(std::string const& what, boost::optional<lexeme> where = boost::optional<lexeme>()) {
    std::ostringstream os;
    if (where) {
        os << where->physical_location.filename << ", line " << where->physical_location.line << ", column "
           << where->physical_location.column << ": ";
    }
    os << what;
    return syntax_error(os.str());
}

lexeme expect(
    lexer& lexer,
    lexeme::e_type type,
    boost::optional<std::string> value = boost::optional<std::string>()
) {
    boost::optional<lexeme> lexeme;
    if ((lexeme = accept(lexer, type, value)))
        return *lexeme;
    else {
        std::ostringstream os;
        os << "Expected ";
        if (value)
            os << *value;
        else
            os << to_string(type);
        os << ", got ";
        if ((lexeme = accept(lexer))) {
            if (value)
                os << lexeme->value;
            else
                os << to_string(lexeme->type);
        } else
            os << "end of input";
        throw error(os.str(), lexeme);
    }
}



std::auto_ptr<numeric_expr> parse_numeric_expr(lexer&);

std::auto_ptr<numeric_expr> parse_factor(lexer& lexer) {
    boost::optional<lexeme> lexeme;

    int sign = 1;
    if (accept(lexer, lexeme::type_symbol, std::string("-")))
        sign = -1;

    if ((lexeme = accept(lexer, lexeme::type_number))) {
        // Parse an numeric constant.
        std::istringstream is(lexeme->value);
        if (lexeme->value.find('.') == std::string::npos) {
            // An integer.
            int value;
            is >> value;
            return std::auto_ptr<numeric_expr>(new constant_expr(sign * value));
        } else {
            // Floating-point value.
            double value;
            is >> value;
            return std::auto_ptr<numeric_expr>(new constant_expr(sign * value));
        }

    } else if ((lexeme = accept(lexer, lexeme::type_word))) {
        // Parse a variable name.
        if (!is_string_identifier(lexeme->value)) {
            if (sign > 0)
                return std::auto_ptr<numeric_expr>(new variable_expr(lexeme->value));
            else
                return std::auto_ptr<numeric_expr>(new arith_expr(
                    std::auto_ptr<numeric_expr>(new constant_expr(-1)),
                    std::auto_ptr<numeric_expr>(new variable_expr(lexeme->value)),
                    arith_expr::operator_times));
        } else {
            throw error("String identifier in numeric expression", lexeme);
        }
    } else if ((lexeme = accept(lexer, lexeme::type_symbol, std::string("(")))) {
        std::auto_ptr<numeric_expr> sub_expression = parse_numeric_expr(lexer);
        expect(lexer, lexeme::type_symbol, std::string(")"));
        return sub_expression;
    } else if ((lexeme = accept(lexer, lexeme::type_string))) {
        // Special-case strings so that we can produce nicer error messages.
        throw error("String literal in numeric expression", lexeme);
    } else
        throw error("Expected an integral constant, a variable name, or an opening parenthesis", accept(lexer));
}

std::auto_ptr<numeric_expr> parse_term(lexer& lexer) {
    std::auto_ptr<numeric_expr> left_side = parse_factor(lexer);

    boost::optional<lexeme> lexeme;
    if ((lexeme = accept(lexer, lexeme::type_symbol, std::string("*")))) {
        std::auto_ptr<numeric_expr> right_side = parse_factor(lexer);
        return std::auto_ptr<numeric_expr>(new arith_expr(left_side, right_side, arith_expr::operator_times));
    } else if ((lexeme = accept(lexer, lexeme::type_symbol, std::string("/")))) {
        std::auto_ptr<numeric_expr> right_side = parse_factor(lexer);
        return std::auto_ptr<numeric_expr>(new arith_expr(left_side, right_side, arith_expr::operator_divides));
    } else if ((lexeme = accept(lexer, lexeme::type_word, std::string("mod")))) {
        std::auto_ptr<numeric_expr> right_side = parse_factor(lexer);
        return std::auto_ptr<numeric_expr>(new arith_expr(left_side, right_side, arith_expr::operator_modulo));
    } else
        return left_side;
}

std::auto_ptr<numeric_expr> parse_integer_expr(lexer& lexer) {
    std::auto_ptr<numeric_expr> left_side = parse_term(lexer);

    boost::optional<lexeme> lexeme;
    if ((lexeme = accept(lexer, lexeme::type_symbol, std::string("+")))) {
        std::auto_ptr<numeric_expr> right_side = parse_integer_expr(lexer);
        return std::auto_ptr<numeric_expr>(new arith_expr(left_side, right_side, arith_expr::operator_plus));
    } else if ((lexeme = accept(lexer, lexeme::type_symbol, std::string("-")))) {
        std::auto_ptr<numeric_expr> right_side = parse_integer_expr(lexer);
        return std::auto_ptr<numeric_expr>(new arith_expr(left_side, right_side, arith_expr::operator_minus));
    } else
        return left_side;
}

std::auto_ptr<numeric_expr> parse_relational_expr(lexer& lexer) {
    std::auto_ptr<numeric_expr> left_side = parse_integer_expr(lexer);

    boost::optional<lexeme> lexeme;
    relational_expr::e_operator op;

    if (accept(lexer, lexeme::type_symbol, std::string("=")))
        op = relational_expr::operator_equals;
    else if (accept(lexer, lexeme::type_symbol, std::string("<>")))
        op = relational_expr::operator_doesnt_equal;
    else if (accept(lexer, lexeme::type_symbol, std::string("<")))
        op = relational_expr::operator_less_than;
    else if (accept(lexer, lexeme::type_symbol, std::string("<=")))
        op = relational_expr::operator_less_equal;
    else if (accept(lexer, lexeme::type_symbol, std::string(">")))
        op = relational_expr::operator_greater_than;
    else if (accept(lexer, lexeme::type_symbol, std::string(">=")))
        op = relational_expr::operator_greater_equal;
    else
        return left_side;  // Not a relational expression.

    std::auto_ptr<numeric_expr> right_side = parse_integer_expr(lexer);
    return std::auto_ptr<numeric_expr>(new relational_expr(left_side, right_side, op));
}

std::auto_ptr<string_expr> parse_string_expr(lexer& lexer);

std::auto_ptr<string_expr> parse_string_atom(lexer& lexer) {
    boost::optional<lexeme> lexeme;

    if ((lexeme = accept(lexer, lexeme::type_string))) {
        return std::auto_ptr<string_expr>(new string_literal_expr(lexeme->value));
    } else if ((lexeme = accept(lexer, lexeme::type_word))) {
        if (is_string_identifier(lexeme->value)) {
            return std::auto_ptr<string_expr>(new string_variable_expr(lexeme->value));
        } else {
            throw error("Expected a string identifier", lexeme);
        }
    } else if (accept(lexer, lexeme::type_symbol, std::string("("))) {
        std::auto_ptr<string_expr> sub_expression = parse_string_expr(lexer);
        expect(lexer, lexeme::type_symbol, std::string(")"));
        return sub_expression;
    } else {
        throw error("Expected a string literal, string identifier or opening parenthesis", next_symbol);
    }
}

std::auto_ptr<string_expr> parse_string_expr(lexer& lexer) {
    std::auto_ptr<string_expr> left_side = parse_string_atom(lexer);

    if (accept(lexer, lexeme::type_symbol, std::string("&"))) {
        std::auto_ptr<string_expr> right_side = parse_string_expr(lexer);
        return std::auto_ptr<string_expr>(new string_concat_expr(left_side, right_side));
    } else {
        return left_side;
    }
}

std::auto_ptr<numeric_expr> parse_numeric_expr(lexer& lexer) {
    // Parse a boolean expression.
    if (accept(lexer, lexeme::type_word, std::string("not"))) {
        std::auto_ptr<numeric_expr> left_side = parse_numeric_expr(lexer);
        return std::auto_ptr<numeric_expr>(new boolean_expr(left_side, std::auto_ptr<numeric_expr>(),
            boolean_expr::operator_not));
    } else {
        std::auto_ptr<numeric_expr> left_side = parse_relational_expr(lexer);

        boolean_expr::e_operator op;
        if (accept(lexer, lexeme::type_word, std::string("and")))
            op = boolean_expr::operator_and;
        else if (accept(lexer, lexeme::type_word, std::string("or")))
            op = boolean_expr::operator_or;
        else  // Not a boolean expression
            return left_side;

        std::auto_ptr<numeric_expr> right_side = parse_integer_expr(lexer);
        return std::auto_ptr<numeric_expr>(new boolean_expr(left_side, right_side, op));
    }
}

std::auto_ptr<printable_expr> parse_expression(lexer& lexer) {
    if (next_symbol && (next_symbol->type == lexeme::type_string
        || (next_symbol->type == lexeme::type_word && is_string_identifier(next_symbol->value))))
    {
        return std::auto_ptr<printable_expr>(parse_string_expr(lexer));
    } else {
        return std::auto_ptr<printable_expr>(parse_numeric_expr(lexer));
    }
}

// If line contains a BLOCK_TERMINATOR, the terminator will be extracted from the lexer, terminating_statement
// set to the apropriate keyword, and the .statement of the return value will be null.
line parse_line(lexer& lexer, std::string& terminating_keyword) {
    line result;
    boost::optional<lexeme> symbol;

do_parse:
    // Accept integral label at the beginning.
    if ((symbol = accept(lexer, lexeme::type_number)))
        result.label = symbol->value;

    if ((symbol = accept(lexer, lexeme::type_word))) {
        if (symbol->value != "rem") {
            // First word can be either a keyword or label.
            if (next_symbol && next_symbol->type == lexeme::type_symbol && next_symbol->value == ":") {
                result.label = symbol->value;
                accept(lexer);  // Consume the ':'.

                // Allow newlines between the label and the actual statement.
                while (next_symbol && next_symbol->type == lexeme::type_end)
                    accept(lexer);

                symbol = expect(lexer, lexeme::type_word);
            }

            if (symbol->value != "rem") {
                parsers_map_t::const_iterator parser = parsers_map.find(symbol->value);
                if (parser != parsers_map.end())
                    result.statement = (parser->second)(lexer);
                else if (std::find(BLOCK_TERMINATORS, BLOCK_TERMINATORS_END, symbol->value) != BLOCK_TERMINATORS_END) {
                    terminating_keyword = symbol->value;
                    return result;
                } else
                    throw error(std::string("Unrecognised keyword: ") + symbol->value, symbol);
            } else {
                // It's a comment, just like below!
                lexer.ignore_line();
                next_symbol = lexer.get_lexeme();
                goto do_parse;
            }
        } else {
            // It's a comment!
            lexer.ignore_line();
            next_symbol = lexer.get_lexeme();
            goto do_parse;  // And go around, parse the next line, pretending the comment never happened.
        }

        expect(lexer, lexeme::type_end);
    } else {
        // This must be an empty line.
        expect(lexer, lexeme::type_end);
        result.statement.reset(new empty_stmt);
    }

    return result;
}

// Extract a block from the stream.  Upon exit, terminating_keyword will be set to the keyword that ended the block, if
// the block ended due to end of input, it will be .empty().
block parse_block(lexer& lexer, std::string& terminating_keyword) {
    block result;

    while (next_symbol) {
        line l = parse_line(lexer, terminating_keyword).copy();
        block::statement_list::iterator statement;
        if (l.statement.get())
            statement = result.statements.insert(result.statements.end(), boost::shared_ptr< ::statement>(l.statement));
        else
            return result;

        if (!l.label.empty())
            result.jump_table.insert(std::make_pair(l.label, statement));
    }

    terminating_keyword.clear();
    return result;
}

std::auto_ptr<statement> parse_if(lexer& lexer) {
    std::auto_ptr<numeric_expr> condition = parse_numeric_expr(lexer);
    expect(lexer, lexeme::type_word, std::string("then"));

    boost::optional<lexeme> lexeme;
    if ((lexeme = accept(lexer, lexeme::type_number)) || (lexeme = accept(lexer, lexeme::type_word))) {
        // There is a label after THEN -- that means this is an expression of the form IF <cond> THEN <label> [ELSE <label>]
        std::string const then_label = lexeme->value;
        std::string else_label = "";

        // See if there is any ELSE part.
        if ((lexeme = accept(lexer, lexeme::type_word, std::string("else")))) {
            if ((lexeme = accept(lexer, lexeme::type_number)))
                else_label = lexeme->value;
            else
                else_label = expect(lexer, lexeme::type_word).value;
        }

        return std::auto_ptr<statement>(new if_goto_stmt(condition, then_label, else_label));
    } else if (next_symbol && next_symbol->type == lexeme::type_end) {
        // THEN is followed by the end of line, we have the block form of IF.
        accept(lexer);  // Eat the newline.

        if_block_stmt::conditions_cont conditions;
        std::vector<block> blocks;

        conditions.push_back(boost::shared_ptr<numeric_expr>(condition));

        std::string keyword = "if";

        do {
            block clause = parse_block(lexer, keyword);
            blocks.push_back(clause);

            if (keyword == "elseif") {
                condition = parse_numeric_expr(lexer);
                expect(lexer, lexeme::type_word, std::string("then"));
                expect(lexer, lexeme::type_end);
                conditions.push_back(boost::shared_ptr<numeric_expr>(condition));
            } else if (keyword != "end" && keyword != "else") {
                std::ostringstream os;
                os << "Unexpected ";
                if (!keyword.empty())
                    os << "keyword " << keyword;
                else
                    os << "end of input";
                os << ", expected ELSE, ELSEIF or END IF";
                throw error(os.str());
            }
        } while (keyword != "end");
        expect(lexer, lexeme::type_word, std::string("if"));  // The whole thing ends with an END IF.

        return std::auto_ptr<statement>(new if_block_stmt(conditions, blocks));
    } else
        throw error("Expected a label or newline after THEN");
}

std::auto_ptr<statement> parse_do(lexer& lexer) {
    expect(lexer, lexeme::type_word, std::string("while"));
    std::auto_ptr<numeric_expr> condition = parse_numeric_expr(lexer);
    std::string terminator;
    block body = parse_block(lexer, terminator);

    if (terminator != "loop")
        throw error(std::string("Expected LOOP, got ") + terminator);

    return std::auto_ptr<statement>(new do_stmt(condition, body));
}

std::auto_ptr<statement> parse_for(lexer& lexer) {
    std::string const variable_name = expect(lexer, lexeme::type_word).value;
    expect(lexer, lexeme::type_symbol, std::string("="));
    std::auto_ptr<numeric_expr> initial_value = parse_numeric_expr(lexer);
    expect(lexer, lexeme::type_word, std::string("to"));
    std::auto_ptr<numeric_expr> final_value = parse_numeric_expr(lexer);

    std::auto_ptr<numeric_expr> step_value;

    if (accept(lexer, lexeme::type_word, std::string("step")))
        step_value = parse_numeric_expr(lexer);
    else
        step_value = std::auto_ptr<numeric_expr>(new constant_expr(1));

    std::string terminator;
    block body = parse_block(lexer, terminator);
    if (terminator == "next") {
        expect(lexer, lexeme::type_word, variable_name);
        return std::auto_ptr<statement>(new for_stmt(variable_name, initial_value, final_value, step_value, body));
    } else
        throw error(std::string("Expected NEXT ") + variable_name + std::string(", got ") + terminator);
}

std::auto_ptr<statement> parse_print(lexer& lexer) {
    print_stmt::expressions_cont expressions;
    if (next_symbol && next_symbol->type != lexeme::type_end) {
        do {
            expressions.push_back(boost::shared_ptr<printable_expr>(parse_expression(lexer)));
        } while (accept(lexer, lexeme::type_symbol, std::string(",")));
    }

    return std::auto_ptr<statement>(new print_stmt(expressions));
}

std::auto_ptr<statement> parse_input(lexer& lexer) {
    std::string const var_name = expect(lexer, lexeme::type_word).value;
    return std::auto_ptr<statement>(new input_stmt(var_name));
}

std::auto_ptr<statement> parse_let(lexer& lexer) {
    std::string const var_name = expect(lexer, lexeme::type_word).value;
    expect(lexer, lexeme::type_symbol, std::string("="));

    if (!is_string_identifier(var_name)) {
        std::auto_ptr<numeric_expr> value = parse_numeric_expr(lexer);
        return std::auto_ptr<statement>(new let_stmt(var_name, value));
    } else {
        std::auto_ptr<string_expr> value = parse_string_expr(lexer);
        return std::auto_ptr<statement>(new let_stmt(var_name, value));
    }
}

std::auto_ptr<statement> parse_goto(lexer& lexer) {
    std::string label;
    boost::optional<lexeme> lexeme;

    if ((lexeme = accept(lexer, lexeme::type_word)) || (lexeme = accept(lexer, lexeme::type_number)))
        label = lexeme->value;
    else
        throw error(std::string("Expected a label"));

    return std::auto_ptr<statement>(new goto_stmt(label));
}

std::auto_ptr<statement> parse_stop(lexer& lexer) {
    return std::auto_ptr<statement>(new stop_stmt);
}

std::auto_ptr<statement> parse_exit(lexer& lexer) {
    std::string const what = expect(lexer, lexeme::type_word).value;
    return std::auto_ptr<statement>(new exit_stmt(what));
}

// Helper object to fill the map of parsers.
struct init_parsers_table {
    init_parsers_table() {
        parsers_map["if"] = &parse_if;
        parsers_map["do"] = &parse_do;
        parsers_map["for"] = &parse_for;
        parsers_map["print"] = &parse_print;
        parsers_map["input"] = &parse_input;
        parsers_map["let"] = &parse_let;
        parsers_map["goto"] = &parse_goto;
        parsers_map["stop"] = &parse_stop;
        parsers_map["exit"] = &parse_exit;
    }
} init_parsers_table_;

}

syntax_error::syntax_error(std::string const& what)
    : std::runtime_error(what)
{ }

bool is_string_identifier(std::string const& identifier) {
    return !identifier.empty() && *identifier.rbegin() == '$';
}

block parse(lexer& lexer) {
    next_symbol = lexer.get_lexeme();
    std::string terminator;
    block b = parse_block(lexer, terminator);
    if (!terminator.empty() && (terminator != "end" || (next_symbol))) {
        std::ostringstream os;
        os << "Unexpected " << terminator << ", expected END or end-of-file";
        throw error(os.str(), next_symbol);
    }

    return b;
}
