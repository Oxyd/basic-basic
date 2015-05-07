#include <iostream>

#include <cassert>
#include "interpreter.hh"

runtime_error::runtime_error(std::string const& what)
    : std::runtime_error(what)
{ }

interpreter::interpreter(block& block)
    : should_stop(false)
    , numeric_variables(&execution_block::numeric_variables)
    , string_variables(&execution_block::string_variables)
{
    enter_block(block);
}

void interpreter::run() {
    should_stop = false;

    while (!blocks.empty() && !should_stop) {
        while (blocks.front().current_statement != blocks.front().block->statements.end() && !should_stop) {
            execution_block& current_block = blocks.front();
            block::statement_list::iterator current = current_block.current_statement;
            ++current_block.current_statement;
            (*current)->execute(*this);
        }

        block_statement* statement = blocks.front().statement;
        exit_block();
        if (statement)
            statement->iterate(*this);
    }
}

void interpreter::jump(std::string const& label) {
    while (!blocks.empty()) {
        block::jump_table_t::iterator target = blocks.front().block->jump_table.find(label);
        if (target != blocks.front().block->jump_table.end()) {
            blocks.front().current_statement = target->second;
            return;
        } else
            exit_block();
    }

    throw runtime_error(std::string("Jump to undefined label ") + label);
}

void interpreter::enter_block(block& block, block_statement* statement) {
    execution_block temp;
    temp.statement = statement;
    temp.block = &block;
    temp.current_statement = block.statements.begin();

    blocks.push_front(temp);
}

void interpreter::exit_block() {
    if (!blocks.empty())
        blocks.pop_front();
}

void interpreter::exit_block(std::string const& name) {
    while (!blocks.empty()) {
        std::string popped_name;  // Name of the just-popped block.
        if (blocks.front().statement)
            popped_name = blocks.front().statement->get_name();

        blocks.pop_front();
        if (popped_name == name)
            return;
    }

    throw runtime_error(std::string("Cannot EXIT ") + name + ": No such block");
}

void interpreter::stop() {
    blocks.clear();
    should_stop = true;
}

void interpreter::set_var_numeric(std::string const& name, number value) {
    set_var(name, numeric_variables, value);
}

void interpreter::set_var_string(std::string const& name, std::string const& value) {
    set_var(name, string_variables, value);
}

number interpreter::get_var_numeric(std::string const& name) {
    return get_var(name, numeric_variables);
}

std::string interpreter::get_var_string(std::string const& name) {
    return get_var(name, string_variables);
}

template <typename VariablesMapT>
bool interpreter::find_var(
    std::string const& name,
    VariablesMapT (execution_block::*variables),
    typename VariablesMapT::iterator& result
) {
    for (execution_block_stack_t::iterator block = blocks.begin(); block != blocks.end(); ++block) {
        typename VariablesMapT::iterator var = ((*block).*variables).find(name);
        if (var != ((*block).*variables).end()) {
            result = var;
            return true;
        }
    }

    return false;
}

template <typename VariablesMapT>
void interpreter::set_var(
    std::string const& name,
    VariablesMapT (execution_block::*variables),
    typename VariablesMapT::mapped_type value
) {
    typename VariablesMapT::iterator var;
    if (find_var(name, variables, var))
        var->second = value;
    else
        (blocks.front().*variables).insert(std::make_pair(name, value));
}

template <typename VariablesMapT>
typename VariablesMapT::mapped_type interpreter::get_var(
    std::string const& name,
    VariablesMapT (execution_block::*variables)
) {
    typename VariablesMapT::iterator var;
    if (find_var(name, variables, var))
        return var->second;
    else
        throw runtime_error(std::string("Variable ") + name + " undefined");
}
