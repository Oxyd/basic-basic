#ifndef PARSER_HH
#define PARSER_HH

#include <stdexcept>
#include <vector>
#include <list>
#include <string>
#include <map>

#include <boost/shared_ptr.hpp>

struct statement;
struct lexer;

struct block {
    typedef std::list<boost::shared_ptr<statement> > statement_list;
    typedef std::map<std::string, statement_list::iterator> jump_table_t;

    statement_list statements;
    jump_table_t jump_table;
};

struct syntax_error : std::runtime_error {
    explicit syntax_error(std::string const& what);
};

bool is_string_identifier(std::string const& ident);

// Parse the stream of lexemes inside the given lexer.
block parse(lexer& lexer);

#endif
