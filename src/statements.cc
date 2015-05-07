#include <sstream>
#include <cassert>
#include <iostream>

#include "parser.hh"
#include "interpreter.hh"
#include "statements.hh"

void statement::execute(interpreter& interpreter) {
    do_execute(interpreter);
}

block_statement::block_statement(std::string const& name)
    : name(name)
{ }

std::string block_statement::get_name() const {
    return name;
}

void block_statement::iterate(interpreter& interpreter) {
    do_iterate(interpreter);
}

std::string printable_expr::get_representation(interpreter& interpreter) const {
    return do_get_representation(interpreter);
}

std::string string_expr::evaluate(interpreter& interpreter) const {
    return do_evaluate(interpreter);
}

std::string string_expr::do_get_representation(interpreter& interpreter) const {
    return do_evaluate(interpreter);
}

string_concat_expr::string_concat_expr(std::auto_ptr<string_expr> left, std::auto_ptr<string_expr> right)
    : left(left)
    , right(right)
{
    assert(this->left.get());
    assert(this->right.get());
}

std::string string_concat_expr::do_evaluate(interpreter& interpreter) const {
    return left->evaluate(interpreter) + right->evaluate(interpreter);
}

string_variable_expr::string_variable_expr(std::string const& var_name)
    : var_name(var_name)
{
    assert(is_string_identifier(var_name));
}

std::string string_variable_expr::do_evaluate(interpreter& interpreter) const {
    return interpreter.get_var_string(var_name);
}

string_literal_expr::string_literal_expr(std::string const& value)
    : value(value)
{ }

std::string string_literal_expr::do_evaluate(interpreter&) const {
    return value;
}

number numeric_expr::evaluate(interpreter& interpreter) const {
    return do_evaluate(interpreter);
}

std::string numeric_expr::do_get_representation(interpreter& interpreter) const {
    std::ostringstream os;
    os << do_evaluate(interpreter);
    return os.str();
}

arith_expr::arith_expr(std::auto_ptr<numeric_expr> left_side, std::auto_ptr<numeric_expr> right_side, e_operator op)
    : left_side(left_side)
    , right_side(right_side)
    , op(op)
{
    assert(this->left_side.get());
    assert(this->right_side.get());
}

number arith_expr::do_evaluate(interpreter& interpreter) const {
    number result = left_side->evaluate(interpreter);

    switch (op) {
    case operator_plus:     result += right_side->evaluate(interpreter); break;
    case operator_minus:    result -= right_side->evaluate(interpreter); break;
    case operator_times:    result *= right_side->evaluate(interpreter); break;
    case operator_modulo:   result %= right_side->evaluate(interpreter); break;
    case operator_divides:  result /= right_side->evaluate(interpreter); break;
    }

    return result;
}

variable_expr::variable_expr(std::string const& name)
    : name(name)
{ }

number variable_expr::do_evaluate(interpreter& interpreter) const {
    return interpreter.get_var_numeric(name);
}

constant_expr::constant_expr(number value)
    : value(value)
{ }

number constant_expr::do_evaluate(interpreter&) const {
    return value;
}

relational_expr::relational_expr(
    std::auto_ptr<numeric_expr> left_side,
    std::auto_ptr<numeric_expr> right_side,
    e_operator op
)
    : left_side(left_side)
    , right_side(right_side)
    , op(op)
{
    assert(this->left_side.get());
    assert(this->right_side.get());
}

number relational_expr::do_evaluate(interpreter& interpreter) const {
    number const left = left_side->evaluate(interpreter);
    number const right = right_side->evaluate(interpreter);

    switch (op) {
    case operator_equals:           return left == right;
    case operator_doesnt_equal:     return left != right;
    case operator_less_than:        return left < right;
    case operator_less_equal:       return left <= right;
    case operator_greater_than:     return left > right;
    case operator_greater_equal:    return left >= right;
    }

    assert(!"Never gets here");
    return 0;
}

boolean_expr::boolean_expr(
    std::auto_ptr<numeric_expr> left_side,
    std::auto_ptr<numeric_expr> right_side,
    e_operator op
)
    : left_side(left_side)
    , right_side(right_side)
    , op(op)
{
    assert(this->left_side.get());
    assert(op == operator_not || this->right_side.get());
}

number boolean_expr::do_evaluate(interpreter& interpreter) const {
    switch (op) {
    case operator_and:
        return left_side->evaluate(interpreter).is_true() && right_side->evaluate(interpreter).is_true();
    case operator_or:
        return left_side->evaluate(interpreter).is_true() || right_side->evaluate(interpreter).is_true();
    case operator_not:
        return !left_side->evaluate(interpreter).is_true();
    }

    assert(!"Never gets here");
    return 0;  // Stupid, stupid compiler.
}

if_goto_stmt::if_goto_stmt(
    std::auto_ptr<numeric_expr> condition,
    std::string const& then_label,
    std::string const& else_label)
    : condition(condition)
    , then_label(then_label)
    , else_label(else_label)
{
    assert(this->condition.get());
}

void if_goto_stmt::do_execute(interpreter& interpreter) {
    if (condition->evaluate(interpreter).is_true())
        interpreter.jump(then_label);
    else if (!else_label.empty())
        interpreter.jump(else_label);
}

if_block_stmt::if_block_stmt(conditions_cont const& conditions, std::vector<block> const& blocks)
    : conditions(conditions)
    , blocks(blocks)
{
    assert(conditions.size() >= 1);
    assert(blocks.size() >= conditions.size() && blocks.size() <= conditions.size() + 1);
}

void if_block_stmt::do_execute(interpreter& interpreter) {
    for (std::size_t i = 0; i < conditions.size(); ++i) {
        if (conditions[i]->evaluate(interpreter).is_true()) {
            interpreter.enter_block(blocks[i]);
            return;
        }
    }

    if (blocks.size() == conditions.size() + 1)
        interpreter.enter_block(blocks[blocks.size() - 1]);
}

do_stmt::do_stmt(std::auto_ptr<numeric_expr> condition, block const& block)
    : block_statement("do")
    , condition(condition)
    , body(block)
{
    assert(this->condition.get());
}

void do_stmt::do_execute(interpreter& interpreter) {
    do_iterate(interpreter);
}

void do_stmt::do_iterate(interpreter& interpreter) {
    if (condition->evaluate(interpreter).is_true())
        interpreter.enter_block(body, this);
}

for_stmt::for_stmt(
    std::string const& variable_name,
    std::auto_ptr<numeric_expr> initial_value,
    std::auto_ptr<numeric_expr> final_value,
    std::auto_ptr<numeric_expr> step, block const& block
)

    : block_statement("for")
    , variable_name(variable_name)
    , initial_expression(initial_value)
    , final_expression(final_value)
    , step_expression(step)
    , body(block)
{
    assert(this->initial_expression.get());
    assert(this->final_expression.get());
    assert(this->step_expression.get());
}

void for_stmt::do_execute(interpreter& interpreter) {
    interpreter.set_var_numeric(variable_name, initial_expression->evaluate(interpreter));
    step = step_expression->evaluate(interpreter);
    final_value = final_expression->evaluate(interpreter);

    if ((step > 0 && interpreter.get_var_numeric(variable_name) <= final_value) ||
            (step < 0 && interpreter.get_var_numeric(variable_name) >= final_value))
        interpreter.enter_block(body, this);
}

void for_stmt::do_iterate(interpreter& interpreter) {
    number iterator_value = interpreter.get_var_numeric(variable_name);
    iterator_value += step;
    interpreter.set_var_numeric(variable_name, iterator_value);

    if ((step > 0 && iterator_value <= final_value) ||
            (step < 0 && iterator_value >= final_value))
        interpreter.enter_block(body, this);
}

print_stmt::print_stmt(expressions_cont const& expressions)
    : expressions(expressions)
{ }

void print_stmt::do_execute(interpreter& interpreter) {
    expressions_cont::const_iterator expr;
    for (expr = expressions.begin(); expr != expressions.end(); ++expr)
        std::cout << (*expr)->get_representation(interpreter);
    std::cout << '\n';
}

input_stmt::input_stmt(std::string const& var_name)
    : var_name(var_name)
{ }

void input_stmt::do_execute(interpreter& interpreter) {
    std::cout << "? ";
    std::string input_line;
    std::getline(std::cin, input_line);

    int input;
    std::istringstream is(input_line);
    if (is >> input)
        interpreter.set_var_numeric(var_name, input);
    else
        throw runtime_error("User input error: expected an integer");
}

let_stmt::let_stmt(std::string const& var_name, std::auto_ptr<numeric_expr> value)
    : var_name(var_name)
    , value_numeric(value)
{
    assert(!is_string_identifier(var_name));
}

let_stmt::let_stmt(std::string const& var_name, std::auto_ptr<string_expr> value)
    : var_name(var_name)
    , value_string(value)
{
    assert(is_string_identifier(var_name));
}

void let_stmt::do_execute(interpreter& interpreter) {
    if (value_numeric.get()) {
        interpreter.set_var_numeric(var_name, value_numeric->evaluate(interpreter));
    } else {
        interpreter.set_var_string(var_name, value_string->evaluate(interpreter));
    }
}

goto_stmt::goto_stmt(std::string const& label)
    : label(label)
{ }

void goto_stmt::do_execute(interpreter& interpreter) {
    interpreter.jump(label);
}

stop_stmt::stop_stmt() { }

void stop_stmt::do_execute(interpreter& interpreter) {
    interpreter.stop();
}

exit_stmt::exit_stmt(std::string const& what)
    : what(what)
{ }

void exit_stmt::do_execute(interpreter& interpreter) {
    interpreter.exit_block(what);
}

empty_stmt::empty_stmt() { }

void empty_stmt::do_execute(interpreter&) { }
