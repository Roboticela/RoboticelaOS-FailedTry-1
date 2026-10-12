#include "Robo/Separator.hpp"

namespace Robo {

void Separator::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& Separator::GetLabel() const {
    return label_;
}

void Separator::SetVariant(SeparatorVariant variant) {
    variant_ = variant;
}

SeparatorVariant Separator::GetVariant() const {
    return variant_;
}

void Separator::SetSize(SeparatorSize size) {
    size_ = size;
}

SeparatorSize Separator::GetSize() const {
    return size_;
}

void Separator::SetTone(SeparatorTone tone) {
    tone_ = tone;
}

SeparatorTone Separator::GetTone() const {
    return tone_;
}

void Separator::SetAlign(SeparatorAlign align) {
    align_ = align;
}

SeparatorAlign Separator::GetAlign() const {
    return align_;
}

void Separator::SetOrientation(SeparatorOrientation orientation) {
    orientation_ = orientation;
}

SeparatorOrientation Separator::GetOrientation() const {
    return orientation_;
}

void Separator::SetInset(bool inset) {
    inset_ = inset;
}

bool Separator::IsInset() const {
    return inset_;
}

void Separator::SetFlush(bool flush) {
    flush_ = flush;
}

bool Separator::IsFlush() const {
    return flush_;
}

void Separator::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool Separator::IsDisabled() const {
    return disabled_;
}

SeparatorVariant ParseSeparatorVariant(const std::string& value) {
    if (value == "subtle") {
        return SeparatorVariant::Subtle;
    }
    if (value == "dashed") {
        return SeparatorVariant::Dashed;
    }
    if (value == "dotted") {
        return SeparatorVariant::Dotted;
    }
    if (value == "fade") {
        return SeparatorVariant::Fade;
    }
    return SeparatorVariant::Solid;
}

SeparatorSize ParseSeparatorSize(const std::string& value) {
    if (value == "sm" || value == "small") {
        return SeparatorSize::Small;
    }
    if (value == "lg" || value == "large") {
        return SeparatorSize::Large;
    }
    return SeparatorSize::Medium;
}

SeparatorTone ParseSeparatorTone(const std::string& value) {
    if (value == "brand") {
        return SeparatorTone::Brand;
    }
    if (value == "success") {
        return SeparatorTone::Success;
    }
    if (value == "warning") {
        return SeparatorTone::Warning;
    }
    if (value == "danger") {
        return SeparatorTone::Danger;
    }
    return SeparatorTone::Default;
}

SeparatorAlign ParseSeparatorAlign(const std::string& value) {
    if (value == "start" || value == "left") {
        return SeparatorAlign::Start;
    }
    if (value == "end" || value == "right") {
        return SeparatorAlign::End;
    }
    return SeparatorAlign::Center;
}

SeparatorOrientation ParseSeparatorOrientation(const std::string& value) {
    if (value == "vertical") {
        return SeparatorOrientation::Vertical;
    }
    return SeparatorOrientation::Horizontal;
}

}  // namespace Robo
