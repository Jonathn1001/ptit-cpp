#include "Money.h"
#include <iomanip>
#include <ostream>

std::ostream& operator<<(std::ostream& os, const Money& m) {
    std::ios::fmtflags f = os.flags();
    os << std::fixed << std::setprecision(2) << m.amount << " " << m.currency;
    os.flags(f);
    return os;
}
