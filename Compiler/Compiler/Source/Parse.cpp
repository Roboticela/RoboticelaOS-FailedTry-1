#include "Robo/Parse.hpp"

namespace Robo {

bool ParseBool(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    return fallback;
}

}  // namespace Robo
