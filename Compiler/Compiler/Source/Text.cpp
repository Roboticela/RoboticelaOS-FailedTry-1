#include "Robo/Text.hpp"

namespace Robo {

Text::Text() = default;

void Text::SetStyle(const TextStyle& style) {
    style_ = style;
}

const TextStyle& Text::GetStyle() const {
    return style_;
}

void Text::ClearRuns() {
    runs_.clear();
}

void Text::AddRun(const TextRun& run) {
    runs_.push_back(run);
}

void Text::AddPlain(const std::string& content) {
    TextRun run;
    run.content = content;
    runs_.push_back(run);
}

const std::vector<TextRun>& Text::GetRuns() const {
    return runs_;
}

}  // namespace Robo
