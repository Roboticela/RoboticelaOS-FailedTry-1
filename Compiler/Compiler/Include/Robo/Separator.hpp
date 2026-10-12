#pragma once

#include <string>

namespace Robo {

enum class SeparatorVariant {
    Solid,
    Subtle,
    Dashed,
    Dotted,
    Fade
};

enum class SeparatorSize {
    Small,
    Medium,
    Large
};

enum class SeparatorTone {
    Default,
    Brand,
    Success,
    Warning,
    Danger
};

enum class SeparatorAlign {
    Start,
    Center,
    End
};

enum class SeparatorOrientation {
    Horizontal,
    Vertical
};

// A thin rule between groups. Separator and Divider are the same control.
class Separator {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetVariant(SeparatorVariant variant);
    SeparatorVariant GetVariant() const;

    void SetSize(SeparatorSize size);
    SeparatorSize GetSize() const;

    void SetTone(SeparatorTone tone);
    SeparatorTone GetTone() const;

    void SetAlign(SeparatorAlign align);
    SeparatorAlign GetAlign() const;

    void SetOrientation(SeparatorOrientation orientation);
    SeparatorOrientation GetOrientation() const;

    void SetInset(bool inset);
    bool IsInset() const;

    void SetFlush(bool flush);
    bool IsFlush() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;

private:
    std::string label_;
    SeparatorVariant variant_ = SeparatorVariant::Solid;
    SeparatorSize size_ = SeparatorSize::Medium;
    SeparatorTone tone_ = SeparatorTone::Default;
    SeparatorAlign align_ = SeparatorAlign::Center;
    SeparatorOrientation orientation_ = SeparatorOrientation::Horizontal;
    bool inset_ = false;
    bool flush_ = false;
    bool disabled_ = false;
};

SeparatorVariant ParseSeparatorVariant(const std::string& value);
SeparatorSize ParseSeparatorSize(const std::string& value);
SeparatorTone ParseSeparatorTone(const std::string& value);
SeparatorAlign ParseSeparatorAlign(const std::string& value);
SeparatorOrientation ParseSeparatorOrientation(const std::string& value);

}  // namespace Robo
