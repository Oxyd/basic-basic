#ifndef INTERPRETER_HH
#define INTERPRETER_HH

#include <string>
#include <map>
#include <deque>
#include <stdexcept>

#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include "parser.hh"
#include "statements.hh"
#include "number.hh"

struct runtime_error : std::runtime_error {
    runtime_error(std::string const& what);
};

class interpreter : boost::noncopyable {
public:
    // Construct an interpreter for a given program.
    explicit interpreter(block& block);

    // Run the program.
    void run();

    void jump(std::string const& label);
    void enter_block(block& block, block_statement* statement = 0);
    void exit_block();
    void exit_block(std::string const& name);
    void stop();

    void set_var_numeric(std::string const& name, number value);
    void set_var_string(std::string const& name, std::string const& value);

    // Throws ::runtime_error if no variable of the given name exists.
    number get_var_numeric(std::string const& name);
    std::string get_var_string(std::string const& name);

private:
    struct execution_block {
        typedef std::map<std::string, number> numeric_variables_map_t;
        typedef std::map<std::string, std::string> string_variables_map_t;

        block_statement*                statement;          // May be 0.
        ::block*                        block;              // Shall not be 0.
        block::statement_list::iterator current_statement;
        numeric_variables_map_t         numeric_variables;
        string_variables_map_t          string_variables;
    };

    typedef std::deque<execution_block> execution_block_stack_t;

    execution_block_stack_t blocks;
    bool                    should_stop;

    // Helpers -- just pass these as the second param to find_var.
    execution_block::numeric_variables_map_t execution_block::*numeric_variables;
    execution_block::string_variables_map_t  execution_block::*string_variables;

    // Find a variable of the given name in the top-most block.  If the variable has been found, result is set to the
    // iterator to it and return value is true; if the variable has not been found, result is left unmodified and return
    // value is false.
    template <typename VariablesMapT>
    bool find_var(std::string const& name,
                  VariablesMapT (execution_block::*variables),
                  typename VariablesMapT::iterator& result);

    // Helper for set_var_* functions.
    template <typename VariablesMapT>
    void set_var(std::string const& name,
                 VariablesMapT (execution_block::*variables),
                 typename VariablesMapT::mapped_type value);

    // Helper for get_var_* functions.
    template <typename VariablesMapT>
    typename VariablesMapT::mapped_type get_var(std::string const& name, VariablesMapT (execution_block::*variables));
};

#endif
