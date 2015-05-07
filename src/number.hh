#ifndef NUMBER_HH
#define NUMBER_HH

#include <ostream>

// A class abstracting the numeric type.  It can be either integer or floating-point -- original BASIC seems not to have
// made a clear distinction between the two.  So make all numbers integers if they are so, but promote them to
// floating-point types automatically -- this class should take care of that.
class number {
public:
    number();
    number(number const& from);

    // These two ctors define an implicit conversion from int/double -- so watch out, should it bite you!
    number(int value);
    number(double value);

    number& operator = (number const& rhs);

    int     get_integral_value() const;
    double  get_floating_point_value() const;
    bool    is_integral() const;

    // Evaluate this number in boolean context, the usual: zero is false, non-zero is true.  I *could* have got funky
    // with safe bool conversion operator, but I chose not to.  (Hint: bool is an *integral* type in C++.)
    bool    is_true() const;

    number& operator += (number const& rhs);
    number& operator -= (number const& rhs);
    number& operator *= (number const& rhs);
    number& operator /= (number const& rhs);  // Will throw ::runtime_error on division-by-zero.
    number& operator %= (number const& rhs);  // Will throw ::runtime_error if either argument is not integral.
    // (See interpreter.hh for definition of ::runtime_error.)

private:
    int     integral_value;
    double  floating_point_value;
    bool    integral;

    // Will set the integral flag to false iff this or other is not integral.
    void check_and_promote(number const& other);
};

number operator - (number const& num);  // Unary minus.
number operator + (number const& lhs, number const& rhs);
number operator - (number const& lhs, number const& rhs);
number operator * (number const& lhs, number const& rhs);
number operator / (number const& lhs, number const& rhs);
number operator % (number const& lhs, number const& rhs);

bool operator == (number const& lhs, number const& rhs);
bool operator != (number const& lhs, number const& rhs);
bool operator < (number const& lhs, number const& rhs);
bool operator <= (number const& lhs, number const& rhs);
bool operator > (number const& lhs, number const& rhs);
bool operator >= (number const& lhs, number const& rhs);

std::ostream& operator << (std::ostream& stream, number const& number);

#endif
