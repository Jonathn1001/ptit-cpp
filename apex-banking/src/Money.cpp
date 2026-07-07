#include "Money.h"
#include <iomanip>
#include <ostream>

std::ostream& operator<<(std::ostream& os, const Money& m) {
    std::ios::fmtflags f = os.flags();
    std::streamsize    p = os.precision();
    os << std::fixed << std::setprecision(2) << m.amount << " " << m.currency;
    os.flags(f);
    os.precision(p);  // precision is separate from format flags — restore it too
    return os;
}
