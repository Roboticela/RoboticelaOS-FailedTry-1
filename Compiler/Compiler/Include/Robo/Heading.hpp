#pragma once

#include "Text.hpp"

namespace Robo {

class Heading : public Text {
public:
    explicit Heading(int level);

    int GetLevel() const;
    static TextStyle DefaultStyle(int level);
    static int ClampLevel(int level);

private:
    int level_ = 1;
};

}  // namespace Robo
