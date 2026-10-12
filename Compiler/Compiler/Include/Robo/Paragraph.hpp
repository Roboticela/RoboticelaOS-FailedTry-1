#pragma once

#include "Text.hpp"

namespace Robo {

class Paragraph : public Text {
public:
    Paragraph();

    static TextStyle DefaultStyle();
};

}  // namespace Robo
