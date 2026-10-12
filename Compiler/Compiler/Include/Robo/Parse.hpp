#pragma once

#include <string>

namespace Robo {

// Parses the flag vocabulary shared by every XML attribute: true/yes/1 and
// false/no/0, falling back to `fallback` for anything else.
bool ParseBool(const std::string& value, bool fallback);

}  // namespace Robo
