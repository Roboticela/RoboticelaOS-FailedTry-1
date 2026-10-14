#include "Robo/SelectionArea.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

constexpr float ITEM_WIDTH = 104.0f;
constexpr float ITEM_HEIGHT = 96.0f;

float Distance(float ax, float ay, float bx, float by) {
    const float dx = bx - ax;
    const float dy = by - ay;
    return std::sqrt(dx * dx + dy * dy);
}

SelectionBounds RectFromPoints(float x0, float y0, float x1, float y1) {
    SelectionBounds rect;
    rect.x = std::min(x0, x1);
    rect.y = std::min(y0, y1);
    rect.width = std::fabs(x1 - x0);
    rect.height = std::fabs(y1 - y0);
    return rect;
}

}  // namespace

bool SelectionBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

bool SelectionBounds::Intersects(const SelectionBounds& other) const {
    return x < other.x + other.width && x + width > other.x && y < other.y + other.height && y + height > other.y;
}

bool SelectionBounds::ContainsRect(const SelectionBounds& other) const {
    return other.x >= x && other.y >= y && other.x + other.width <= x + width && other.y + other.height <= y + height;
}

void SelectionAreaItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& SelectionAreaItem::GetLabel() const {
    return label_;
}

void SelectionAreaItem::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

float SelectionAreaItem::GetX() const {
    return x_;
}

float SelectionAreaItem::GetY() const {
    return y_;
}

void SelectionAreaItem::SetSize(float width, float height) {
    if (width > 1.0f) {
        width_ = width;
    }
    if (height > 1.0f) {
        height_ = height;
    }
}

float SelectionAreaItem::GetWidth() const {
    return width_;
}

float SelectionAreaItem::GetHeight() const {
    return height_;
}

void SelectionAreaItem::SetSelected(bool selected) {
    selected_ = selected;
}

bool SelectionAreaItem::IsSelected() const {
    return selected_;
}

void SelectionAreaItem::SetFocused(bool focused) {
    focused_ = focused;
}

bool SelectionAreaItem::IsFocused() const {
    return focused_;
}

void SelectionAreaItem::SetLocked(bool locked) {
    locked_ = locked;
}

bool SelectionAreaItem::IsLocked() const {
    return locked_;
}

void SelectionAreaItem::SetBounds(const SelectionBounds& bounds) {
    bounds_ = bounds;
}

const SelectionBounds& SelectionAreaItem::GetBounds() const {
    return bounds_;
}

void SelectionArea::SetInclusion(SelectionInclusion inclusion) {
    inclusion_ = inclusion;
}

void SelectionArea::SetCombine(SelectionCombine combine) {
    combine_ = combine;
}

void SelectionArea::SetThreshold(float threshold) {
    threshold_ = std::max(1.0f, threshold);
}

void SelectionArea::SetKeyboardFocus(bool focused) {
    keyboardFocus_ = focused;
}

bool SelectionArea::HasKeyboardFocus() const {
    return keyboardFocus_;
}

void SelectionArea::AddItem(SelectionAreaItem item) {
    if (item.GetWidth() <= 1.0f) {
        item.SetSize(ITEM_WIDTH, item.GetHeight());
    }
    if (item.GetHeight() <= 1.0f) {
        item.SetSize(item.GetWidth(), ITEM_HEIGHT);
    }
    if (item.IsSelected() && !item.IsLocked() && focus_ < 0) {
        focus_ = static_cast<int>(items_.size());
        rangeAnchor_ = focus_;
        item.SetFocused(true);
    }
    items_.push_back(std::move(item));
}

std::vector<SelectionAreaItem>& SelectionArea::Items() {
    return items_;
}

const std::vector<SelectionAreaItem>& SelectionArea::Items() const {
    return items_;
}

void SelectionArea::SetBounds(const SelectionBounds& bounds) {
    bounds_ = bounds;
}

const SelectionBounds& SelectionArea::GetBounds() const {
    return bounds_;
}

void SelectionArea::SetPixelScale(float pixelScale) {
    pixelScale_ = pixelScale > 0.0f ? pixelScale : 1.0f;
}

SelectionGesture SelectionArea::GetGesture() const {
    return gesture_;
}

bool SelectionArea::HasMarquee() const {
    return gesture_ == SelectionGesture::Marquee;
}

SelectionBounds SelectionArea::MarqueeRect() const {
    return NormalizedRect();
}

int SelectionArea::SelectedCount() const {
    int count = 0;
    for (const SelectionAreaItem& item : items_) {
        if (item.IsSelected()) {
            ++count;
        }
    }
    return count;
}

bool SelectionArea::Eligible(int index) const {
    return index >= 0 && index < static_cast<int>(items_.size()) && !items_[static_cast<std::size_t>(index)].IsLocked();
}

bool SelectionArea::Hit(float x, float y, int& index) const {
    for (int item = static_cast<int>(items_.size()) - 1; item >= 0; --item) {
        const SelectionAreaItem& entry = items_[static_cast<std::size_t>(item)];
        if (entry.IsLocked()) {
            continue;
        }
        if (entry.GetBounds().Contains(x, y)) {
            index = item;
            return true;
        }
    }
    index = -1;
    return false;
}

SelectionCombine SelectionArea::CombineFor(int modifiers) const {
    if ((modifiers & 8) != 0) {
        return SelectionCombine::Subtract;
    }
    if ((modifiers & 2) != 0 || (modifiers & 4) != 0) {
        return SelectionCombine::Add;
    }
    if ((modifiers & 1) != 0) {
        return SelectionCombine::Toggle;
    }
    return combine_;
}

void SelectionArea::TakeSnapshot() {
    snapshot_.resize(items_.size());
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        const SelectionAreaItem& item = items_[static_cast<std::size_t>(index)];
        ItemSnapshot& snap = snapshot_[static_cast<std::size_t>(index)];
        snap.selected = item.IsSelected();
        snap.x = item.GetX();
        snap.y = item.GetY();
        snap.boundsX = item.GetBounds().x;
        snap.boundsY = item.GetBounds().y;
    }
}

void SelectionArea::RestoreSnapshot() {
    const int count = std::min(static_cast<int>(items_.size()), static_cast<int>(snapshot_.size()));
    for (int index = 0; index < count; ++index) {
        SelectionAreaItem& item = items_[static_cast<std::size_t>(index)];
        const ItemSnapshot& snap = snapshot_[static_cast<std::size_t>(index)];
        item.SetSelected(snap.selected && !item.IsLocked());
        item.SetPosition(snap.x, snap.y);
        SelectionBounds bounds = item.GetBounds();
        bounds.x = snap.boundsX;
        bounds.y = snap.boundsY;
        item.SetBounds(bounds);
    }
}

void SelectionArea::ClearSelection() {
    for (SelectionAreaItem& item : items_) {
        item.SetSelected(false);
    }
}

void SelectionArea::ReplaceWith(int index) {
    ClearSelection();
    if (!Eligible(index)) {
        return;
    }
    items_[static_cast<std::size_t>(index)].SetSelected(true);
    FocusOnly(index);
    rangeAnchor_ = index;
}

void SelectionArea::ToggleIndex(int index) {
    if (!Eligible(index)) {
        return;
    }
    SelectionAreaItem& item = items_[static_cast<std::size_t>(index)];
    item.SetSelected(!item.IsSelected());
    FocusOnly(index);
    rangeAnchor_ = index;
}

void SelectionArea::SelectRange(int from, int to) {
    if (!Eligible(from) || !Eligible(to)) {
        return;
    }
    const int start = std::min(from, to);
    const int end = std::max(from, to);
    ClearSelection();
    for (int index = start; index <= end; ++index) {
        if (Eligible(index)) {
            items_[static_cast<std::size_t>(index)].SetSelected(true);
        }
    }
    FocusOnly(to);
}

void SelectionArea::SelectAll() {
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        if (Eligible(index)) {
            items_[static_cast<std::size_t>(index)].SetSelected(true);
        }
    }
    if (focus_ < 0 && !items_.empty()) {
        FocusOnly(0);
        rangeAnchor_ = 0;
    }
}

void SelectionArea::FocusOnly(int index) {
    focus_ = index;
    for (int item = 0; item < static_cast<int>(items_.size()); ++item) {
        items_[static_cast<std::size_t>(item)].SetFocused(item == index);
    }
}

SelectionBounds SelectionArea::NormalizedRect() const {
    return RectFromPoints(anchorX_, anchorY_, pointerX_, pointerY_);
}

void SelectionArea::ApplyMarquee() {
    const SelectionBounds rect = NormalizedRect();
    const bool empty = rect.width < 1.0f && rect.height < 1.0f;
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        SelectionAreaItem& item = items_[static_cast<std::size_t>(index)];
        if (item.IsLocked()) {
            item.SetSelected(false);
            continue;
        }
        const bool was = index < static_cast<int>(snapshot_.size()) && snapshot_[static_cast<std::size_t>(index)].selected;
        bool inside = false;
        if (!empty) {
            if (inclusion_ == SelectionInclusion::Contain) {
                inside = rect.ContainsRect(item.GetBounds());
            } else {
                inside = rect.Intersects(item.GetBounds());
            }
        }
        bool selected = was;
        if (dragCombine_ == SelectionCombine::Replace) {
            selected = inside;
        } else if (dragCombine_ == SelectionCombine::Add) {
            selected = was || inside;
        } else if (dragCombine_ == SelectionCombine::Subtract) {
            selected = was && !inside;
        } else {
            selected = was != inside;
        }
        item.SetSelected(selected);
    }
}

bool SelectionArea::PointerDown(float x, float y, bool fromTouch, int clicks, int modifiers) {
    if (!bounds_.Contains(x, y)) {
        return false;
    }
    if (gesture_ != SelectionGesture::Idle) {
        PointerUp(x, y);
    }
    keyboardFocus_ = true;
    fromTouch_ = fromTouch;
    anchorX_ = x;
    anchorY_ = y;
    pointerX_ = x;
    pointerY_ = y;
    pressedItem_ = -1;
    dragCombine_ = CombineFor(modifiers);
    TakeSnapshot();
    int hit = -1;
    if (Hit(x, y, hit)) {
        const bool shift = (modifiers & 1) != 0;
        const bool command = (modifiers & 2) != 0 || (modifiers & 4) != 0;
        if (clicks >= 2) {
            if (!items_[static_cast<std::size_t>(hit)].IsSelected()) {
                ReplaceWith(hit);
            } else {
                FocusOnly(hit);
            }
            gesture_ = SelectionGesture::Idle;
            return true;
        }
        if (shift && rangeAnchor_ >= 0) {
            SelectRange(rangeAnchor_, hit);
        } else if (command) {
            ToggleIndex(hit);
        } else if (!items_[static_cast<std::size_t>(hit)].IsSelected()) {
            ReplaceWith(hit);
        } else {
            FocusOnly(hit);
        }
        pressedItem_ = hit;
    }
    gesture_ = SelectionGesture::Pending;
    return true;
}

bool SelectionArea::PointerMove(float x, float y) {
    if (gesture_ == SelectionGesture::Idle) {
        return false;
    }
    pointerX_ = x;
    pointerY_ = y;
    const float limit = threshold_ * pixelScale_ * (fromTouch_ ? 2.5f : 1.0f);
    if (gesture_ == SelectionGesture::Pending) {
        if (Distance(anchorX_, anchorY_, pointerX_, pointerY_) < limit) {
            return false;
        }
        if (pressedItem_ >= 0) {
            gesture_ = SelectionGesture::Move;
            moveAnchorX_ = pointerX_;
            moveAnchorY_ = pointerY_;
            TakeSnapshot();
        } else {
            gesture_ = SelectionGesture::Marquee;
            ApplyMarquee();
        }
        return true;
    }
    if (gesture_ == SelectionGesture::Marquee) {
        ApplyMarquee();
        return true;
    }
    const float scale = pixelScale_ > 0.0f ? pixelScale_ : 1.0f;
    const float dx = (pointerX_ - moveAnchorX_) / scale;
    const float dy = (pointerY_ - moveAnchorY_) / scale;
    for (int index = 0; index < static_cast<int>(items_.size()) && index < static_cast<int>(snapshot_.size()); ++index) {
        SelectionAreaItem& item = items_[static_cast<std::size_t>(index)];
        const ItemSnapshot& snap = snapshot_[static_cast<std::size_t>(index)];
        if (!snap.selected || item.IsLocked()) {
            continue;
        }
        item.SetPosition(snap.x + dx, snap.y + dy);
        SelectionBounds bounds = item.GetBounds();
        bounds.x = snap.boundsX + (pointerX_ - moveAnchorX_);
        bounds.y = snap.boundsY + (pointerY_ - moveAnchorY_);
        item.SetBounds(bounds);
    }
    return true;
}

bool SelectionArea::PointerUp(float x, float y) {
    if (gesture_ == SelectionGesture::Idle) {
        return false;
    }
    pointerX_ = x;
    pointerY_ = y;
    const bool pending = gesture_ == SelectionGesture::Pending;
    const bool marquee = gesture_ == SelectionGesture::Marquee;
    if (marquee) {
        ApplyMarquee();
    } else if (pending && pressedItem_ < 0 && dragCombine_ == SelectionCombine::Replace) {
        ClearSelection();
        focus_ = -1;
        for (SelectionAreaItem& item : items_) {
            item.SetFocused(false);
        }
    }
    gesture_ = SelectionGesture::Idle;
    pressedItem_ = -1;
    return true;
}

bool SelectionArea::LongPress(float x, float y) {
    if (!bounds_.Contains(x, y)) {
        return false;
    }
    int hit = -1;
    if (!Hit(x, y, hit)) {
        return false;
    }
    keyboardFocus_ = true;
    ToggleIndex(hit);
    gesture_ = SelectionGesture::Idle;
    return true;
}

bool SelectionArea::Cancel() {
    if (gesture_ == SelectionGesture::Idle) {
        if (SelectedCount() == 0) {
            return false;
        }
        ClearSelection();
        return true;
    }
    RestoreSnapshot();
    gesture_ = SelectionGesture::Idle;
    pressedItem_ = -1;
    return true;
}

bool SelectionArea::MoveFocus(int direction, bool extend) {
    if (items_.empty()) {
        return false;
    }
    int from = focus_;
    if (from < 0) {
        from = direction == 0 || direction == 2 ? static_cast<int>(items_.size()) : -1;
    }
    const bool horizontal = direction == 0 || direction == 1;
    const float sign = (direction == 1 || direction == 3) ? 1.0f : -1.0f;
    int best = -1;
    float bestScore = 0.0f;
    const SelectionBounds& origin = from >= 0 ? items_[static_cast<std::size_t>(from)].GetBounds() : bounds_;
    const float originX = origin.x + origin.width * 0.5f;
    const float originY = origin.y + origin.height * 0.5f;
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        if (index == from || !Eligible(index)) {
            continue;
        }
        const SelectionBounds& bounds = items_[static_cast<std::size_t>(index)].GetBounds();
        const float dx = (bounds.x + bounds.width * 0.5f) - originX;
        const float dy = (bounds.y + bounds.height * 0.5f) - originY;
        const float along = (horizontal ? dx : dy) * sign;
        const float cross = std::fabs(horizontal ? dy : dx);
        if (along <= 4.0f * pixelScale_) {
            continue;
        }
        const float score = along + cross * 2.0f;
        if (best < 0 || score < bestScore) {
            best = index;
            bestScore = score;
        }
    }
    if (best < 0) {
        return false;
    }
    if (extend) {
        if (rangeAnchor_ < 0) {
            rangeAnchor_ = from >= 0 ? from : best;
        }
        items_[static_cast<std::size_t>(best)].SetSelected(true);
        FocusOnly(best);
    } else {
        ReplaceWith(best);
    }
    return true;
}

bool SelectionArea::KeyDown(const std::string& key, bool shift, int modifiers) {
    if (!keyboardFocus_ || gesture_ == SelectionGesture::Marquee || gesture_ == SelectionGesture::Move) {
        if ((key == "Escape" || key == "Esc") && gesture_ != SelectionGesture::Idle) {
            return Cancel();
        }
        return false;
    }
    const bool command = (modifiers & 2) != 0 || (modifiers & 4) != 0;
    if ((key == "a" || key == "A") && command) {
        SelectAll();
        return true;
    }
    if (key == "Escape" || key == "Esc") {
        return Cancel();
    }
    if (key == " " || key == "Space") {
        if (Eligible(focus_)) {
            ToggleIndex(focus_);
            return true;
        }
        return false;
    }
    if (key == "Enter" || key == "Return") {
        return Eligible(focus_);
    }
    if (key == "Home") {
        int first = -1;
        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            if (Eligible(index)) {
                first = index;
                break;
            }
        }
        if (first < 0) {
            return false;
        }
        if (shift && rangeAnchor_ >= 0) {
            SelectRange(rangeAnchor_, first);
        } else {
            ReplaceWith(first);
        }
        return true;
    }
    if (key == "End") {
        int last = -1;
        for (int index = static_cast<int>(items_.size()) - 1; index >= 0; --index) {
            if (Eligible(index)) {
                last = index;
                break;
            }
        }
        if (last < 0) {
            return false;
        }
        if (shift && rangeAnchor_ >= 0) {
            SelectRange(rangeAnchor_, last);
        } else {
            ReplaceWith(last);
        }
        return true;
    }
    if (key == "Tab") {
        if (items_.empty()) {
            return false;
        }
        int index = focus_;
        const int step = shift ? -1 : 1;
        for (int tries = 0; tries < static_cast<int>(items_.size()); ++tries) {
            index += step;
            if (index < 0) {
                index = static_cast<int>(items_.size()) - 1;
            } else if (index >= static_cast<int>(items_.size())) {
                index = 0;
            }
            if (Eligible(index)) {
                FocusOnly(index);
                return true;
            }
        }
        return false;
    }
    int direction = -1;
    if (key == "ArrowLeft") {
        direction = 0;
    } else if (key == "ArrowRight") {
        direction = 1;
    } else if (key == "ArrowUp") {
        direction = 2;
    } else if (key == "ArrowDown") {
        direction = 3;
    }
    if (direction < 0) {
        return false;
    }
    return MoveFocus(direction, shift);
}

}  // namespace Robo
