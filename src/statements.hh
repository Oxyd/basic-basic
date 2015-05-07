#ifndef STATEMENTS_HH
#define STATEMENTS_HH

#include <memory>
#include <vector>
#include <string>

#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "number.hh"
#include "parser.hh"

struct interpreter;

class statement : boost::noncopyable {
public:
    virtual ~statement() { }
    void execute(interpreter& interpreter);

private:
    virtual void do_execute(interpreter& interpreter) = 0;
};

// Statement with a body, like for, or while.
class block_statement : public statement {
public:
    explicit block_statement(std::string const& name);

    std::string get_name() const;

    // Perform next iteration of the implied block.
    void iterate(interpreter& interpreter);

private:
    std::string name;

    virtual void do_iterate(interpreter& interpreter) = 0;
};

class printable_expr : boost::noncopyable {
public:
    virtual ~printable_expr() { }

    std::string get_representation(interpreter& interpreter) const;

private:
    virtual std::string do_get_representation(interpreter& interpreter) const = 0;
};

class string_expr : public printable_expr {
public:
    std::string evaluate(interpreter& interpreter) const;

private:
    virtual std::string do_evaluate(interpreter&) const = 0;
    virtual std::string do_get_representation(interpreter& interpreter) const;
};

class string_concat_expr : public string_expr {
public:
    string_concat_expr(std::auto_ptr<string_expr> left, std::auto_ptr<string_expr> right);

private:
    virtual std::string do_evaluate(interpreter& interpreter) const;

    std::auto_ptr<string_expr> left, right;
};

class string_variable_expr : public string_expr {
public:
    explicit string_variable_expr(std::string const& var_name);

private:
    virtual std::string do_evaluate(interpreter& interpreter) const;

    std::string var_name;
};

class string_literal_expr : public string_expr {
public:
    explicit string_literal_expr(std::string const& value);

private:
    virtual std::string do_evaluate(interpreter&) const;

    std::string value;
};

// Abstract base class to help distinguish between string and numeric expressions.
class numeric_expr : public printable_expr {
public:
    number evaluate(interpreter& interpreter) const;

private:
    virtual number do_evaluate(interpreter& interpreter) const = 0;
    virtual std::string do_get_representation(interpreter& interpreter) const;
};

class arith_expr : public numeric_expr {
public:
    enum e_operator { operator_plus, operator_minus, operator_times, operator_divides, operator_modulo };

    arith_expr(std::auto_ptr<numeric_expr> left_side, std::auto_ptr<numeric_expr> right_side, e_operator op);

    number evaluate(interpreter& interpreter) const;

private:
    virtual number do_evaluate(interpreter& interpreter) const;

    std::auto_ptr<numeric_expr> left_side, right_side;
    e_operator op;
};

class variable_expr : public numeric_expr {
public:
    explicit variable_expr(std::string const& name);

private:
    virtual number do_evaluate(interpreter& interpreter) const;

    std::string const name;
};

class constant_expr : public numeric_expr {
public:
    explicit constant_expr(number value);

private:
    virtual number do_evaluate(interpreter& interpreter) const;

    number const value;
};

class relational_expr : public numeric_expr {
public:
    enum e_operator {
        operator_equals, operator_doesnt_equal, operator_less_than, operator_less_equal, operator_greater_than,
        operator_greater_equal
    };
    relational_expr(std::auto_ptr<numeric_expr> left_side, std::auto_ptr<numeric_expr> right_side, e_operator op);

private:
    virtual number do_evaluate(interpreter&) const;

    std::auto_ptr<numeric_expr> left_side, right_side;
    e_operator op;
};

class boolean_expr : public numeric_expr {
public:
    enum e_operator { operator_and, operator_or, operator_not };

    // right_side is null if and only if op == operator_not.
    boolean_expr(std::auto_ptr<numeric_expr> left_side, std::auto_ptr<numeric_expr> right_side, e_operator op);

private:
    virtual number do_evaluate(interpreter& interpreter) const;

    std::auto_ptr<numeric_expr> left_side, right_side;
    e_operator op;
};

// A statement of the form "IF <condition> THEN <label> [ELSE label]"
class if_goto_stmt : public statement {
public:
    if_goto_stmt(
        std::auto_ptr<numeric_expr> condition,
        std::string const& then_label,
        std::string const& else_label = "");

private:
    virtual void do_execute(interpreter& interpreter);

    std::auto_ptr<numeric_expr> condition;
    std::string const then_label, else_label;
};

// A statement of the form "IF <condition> THEN <block> [ELSEIF <condition> <block> [ ...]] [ELSE <block>]
class if_block_stmt : public statement {
public:
    typedef std::vector<boost::shared_ptr<numeric_expr> > conditions_cont;

    // conditions.size() shall be at least one and conditions.size() shall be at least blocks.size() and blocks.size() + 1
    // at most.  conditions[0] is the condition of the IF itself, and blocks[0] is the corresponding block.  For all i from
    // 1 to conditions.size() - 1, conditions[i] is the condition of the i'th ELSEIF clause, with blocks[i] being the
    // apropriate block.  blocks[labels.size()], if valid, is the block of the ELSE clause.
    if_block_stmt(conditions_cont const& conditions, std::vector<block> const& blocks);

private:
    virtual void do_execute(interpreter& interpreter);

    conditions_cont conditions;
    std::vector<block> blocks;
};

class do_stmt : public block_statement {
public:
    do_stmt(std::auto_ptr<numeric_expr> condition, block const& block);

private:
    virtual void do_execute(interpreter& interpreter);
    virtual void do_iterate(interpreter& interpreter);

    std::auto_ptr<numeric_expr> condition;
    block body;
};

class for_stmt : public block_statement {
public:
    for_stmt(
        std::string const& variable_name,
        std::auto_ptr<numeric_expr> initial_value, std::auto_ptr<numeric_expr> final_value,
        std::auto_ptr<numeric_expr> step, block const& block);

private:
    virtual void do_execute(interpreter& interpreter);
    virtual void do_iterate(interpreter& interpreter);

    std::string const variable_name;
    std::auto_ptr<numeric_expr> initial_expression;
    std::auto_ptr<numeric_expr> final_expression;
    std::auto_ptr<numeric_expr> step_expression;
    block body;

    number final_value;
    number step;
};

class print_stmt : public statement {
public:
    typedef std::vector<boost::shared_ptr<printable_expr> > expressions_cont;

    explicit print_stmt(expressions_cont const& expressions);

private:
    virtual void do_execute(interpreter& interpreter);

    expressions_cont expressions;
};

class input_stmt : public statement {
public:
    explicit input_stmt(std::string const& var_name);

private:
    virtual void do_execute(interpreter& interpreter);

    std::string const var_name;
};

class let_stmt : public statement {
public:
    let_stmt(std::string const& var_name, std::auto_ptr<numeric_expr> value);
    let_stmt(std::string const& var_name, std::auto_ptr<string_expr> value);

private:
    virtual void do_execute(interpreter& interpreter);

    std::string const var_name;

    // One and exactly one of these two shall be null.
    std::auto_ptr<numeric_expr> value_numeric;
    std::auto_ptr<string_expr> value_string;
};

class goto_stmt : public statement {
public:
    explicit goto_stmt(std::string const& label);

private:
    virtual void do_execute(interpreter& interpreter);

    std::string const label;
};

class stop_stmt : public statement {
public:
    stop_stmt();

private:
    virtual void do_execute(interpreter& interpreter);
};

class exit_stmt : public statement {
public:
    explicit exit_stmt(std::string const& what);

private:
    virtual void do_execute(interpreter& interpreter);

    std::string const what;
};

class empty_stmt : public statement {
public:
    empty_stmt();

private:
    virtual void do_execute(interpreter&);
};

#endif
