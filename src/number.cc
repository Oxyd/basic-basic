#include <limits>
#include <cmath>
#include <iomanip>

#include "interpreter.hh"
#include "number.hh"

number::number()
    : integral_value(0)
    , floating_point_value(0.0)
    , integral(true)
{ }

number::number(number const& from)
    : integral_value(from.integral_value)
    , floating_point_value(from.floating_point_value)
    , integral(from.integral)
{ }

number::number(int value)
    : integral_value(value)
    , floating_point_value(value)
    , integral(true)
{ }

number::number(double value)
    : integral_value(static_cast<int>(value))
    , floating_point_value(value)
    , integral(false)
{ }

number& number::operator = (number const& rhs) {
    if (this != &rhs) {
        integral_value = rhs.integral_value;
        floating_point_value = rhs.floating_point_value;
        integral = rhs.integral;
    }
    return *this;
}

int number::get_integral_value() const {
    return integral_value;
}

double number::get_floating_point_value() const {
    return floating_point_value;
}

bool number::is_integral() const {
    return integral;
}

bool number::is_true() const {
    double const EPSILON = std::numeric_limits<double>::epsilon();
    return (integral && integral_value) || std::fabs(floating_point_value) >= EPSILON;
}

number& number::operator += (number const& rhs) {
    integral_value += rhs.integral_value;
    floating_point_value += rhs.floating_point_value;
    check_and_promote(rhs);
    return *this;
}

number& number::operator -= (number const& rhs) {
    integral_value -= rhs.integral_value;
    floating_point_value -= rhs.floating_point_value;
    check_and_promote(rhs);
    return *this;
}

number& number::operator *= (number const& rhs) {
    integral_value *= rhs.integral_value;
    floating_point_value *= rhs.floating_point_value;
    check_and_promote(rhs);
    return *this;
}

number& number::operator /= (number const& rhs) {
    if (rhs.get_integral_value() != 0) {
        // It's getting a little tricker here -- if both sides are integers and they are divisible, the result is also
        // an integer, in all other cases the result is floating-point.
        if (is_integral() && rhs.is_integral() && integral_value % rhs.integral_value == 0) {
            integral = true;
        } else {
            integral = false;
        }

        integral_value /= rhs.integral_value;
        floating_point_value /= rhs.floating_point_value;
    } else {
        throw runtime_error("Division by zero");
    }

    return *this;
}

number& number::operator %= (number const& rhs) {
    if (is_integral() && rhs.is_integral()) {
        integral_value %= rhs.get_integral_value();
        floating_point_value = integral_value;
    } else {
        throw runtime_error("Modulo operation is only defined on whole number types.");
    }
    return *this;
}

void number::check_and_promote(number const& other) {
    if (is_integral() && other.is_integral()) {
        integral = true;
    } else {
        integral = false;
    }
}

number operator - (number const& num) {
    if (num.is_integral()) {
        return number(-num.get_integral_value());
    } else {
        return number(-num.get_floating_point_value());
    }
}

number operator + (number const& lhs, number const& rhs) {
    number temp = lhs;
    temp += rhs;
    return temp;
}

number operator - (number const& lhs, number const& rhs) {
    number temp = lhs;
    temp -= rhs;
    return temp;
}

number operator * (number const& lhs, number const& rhs) {
    number temp = lhs;
    temp *= rhs;
    return temp;
}

number operator / (number const& lhs, number const& rhs) {
    number temp = lhs;
    temp /= rhs;
    return temp;
}

bool operator == (number const& lhs, number const& rhs) {
    if (lhs.is_integral() && rhs.is_integral()) {
        return lhs.get_integral_value() == rhs.get_integral_value();
    } else {
        double const EPSILON = std::numeric_limits<double>::epsilon();
        return std::fabs(lhs.get_floating_point_value() - rhs.get_floating_point_value()) < EPSILON;
    }
}

bool operator != (number const& lhs, number const& rhs) {
    return !(lhs == rhs);
}

bool operator < (number const& lhs, number const& rhs) {
    if (lhs.is_integral() && rhs.is_integral()) {
        return lhs.get_integral_value() < rhs.get_integral_value();
    } else {
        return lhs.get_floating_point_value() < rhs.get_floating_point_value();
    }
}

bool operator <= (number const& lhs, number const& rhs) {
    return lhs < rhs || lhs == rhs;
}

bool operator > (number const& lhs, number const& rhs) {
    return !(lhs <= rhs);
}

bool operator >= (number const& lhs, number const& rhs) {
    return !(lhs < rhs);
}

std::ostream& operator << (std::ostream& stream, number const& num) {
    if (num.is_integral()) {
        return stream << num.get_integral_value();
    } else {
        return stream << std::showpoint << num.get_floating_point_value();
    }
}
