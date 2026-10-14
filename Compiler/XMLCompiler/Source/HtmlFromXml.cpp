#include "HtmlFromXml.hpp"
#include "XmlParser.hpp"

#include "Robo/Color.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/TextAlign.hpp"

#include <cstdlib>
#include <sstream>

namespace Robo {

namespace {

bool GetAttribute(const XmlNode& node, const std::string& key, std::string& value) {
    const auto it = node.attributes.find(key);
    if (it == node.attributes.end()) {
        return false;
    }
    value = it->second;
    return true;
}

std::string FormatDimensionCss(const std::string& value) {
    if (value.empty() || value == "auto") {
        return "auto";
    }
    if (value.find('%') != std::string::npos || value.find("px") != std::string::npos ||
        value.find("rem") != std::string::npos || value.find("em") != std::string::npos ||
        value.find("vw") != std::string::npos || value.find("vh") != std::string::npos) {
        return value;
    }
    return value + "px";
}

}  // namespace

std::string HtmlFromXml::EscapeHtml(const std::string& value) const {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            default:
                result.push_back(ch);
                break;
        }
    }
    return result;
}

std::string HtmlFromXml::StyleFromAttributes(const XmlNode& node, bool isBlock) const {
    std::ostringstream style;
    std::string value;

    style << "font-family:'Inter',sans-serif;";

    if (GetAttribute(node, "fontSize", value)) {
        style << "font-size:" << value << "px;";
    }
    if (GetAttribute(node, "color", value)) {
        style << "color:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "fontWeight", value)) {
        style << "font-weight:" << FontWeightToCss(ParseFontWeight(value)) << ";";
    }
    if (GetAttribute(node, "fontStyle", value)) {
        style << "font-style:" << FontStyleToCss(ParseFontStyle(value)) << ";";
    }
    if (GetAttribute(node, "textDecoration", value)) {
        style << "text-decoration:" << TextDecorationToCss(ParseTextDecoration(value)) << ";";
    }
    if (GetAttribute(node, "letterSpacing", value)) {
        style << "letter-spacing:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    if (isBlock) {
        if (GetAttribute(node, "align", value)) {
            style << "text-align:" << TextAlignToCss(ParseTextAlign(value)) << ";";
        }
        if (GetAttribute(node, "wrap", value)) {
            if (value == "false" || value == "0" || value == "no") {
                style << "white-space:nowrap;overflow:hidden;text-overflow:ellipsis;";
            } else {
                style << "white-space:normal;overflow-wrap:anywhere;";
            }
        } else {
            style << "white-space:normal;overflow-wrap:anywhere;";
        }
        if (GetAttribute(node, "lineHeight", value)) {
            style << "line-height:" << value << ";";
        }
        if (GetAttribute(node, "width", value)) {
            style << "width:" << value << ";";
        }
        if (GetAttribute(node, "maxWidth", value)) {
            style << "max-width:" << value << ";";
        }
        style << "margin:0 0 0.75rem 0;";
    }

    return style.str();
}

std::string HtmlFromXml::BuildSpanHtml(const XmlNode& node) const {
    std::ostringstream html;
    html << "<span style=\"" << StyleFromAttributes(node, false) << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
        }
    }
    html << "</span>";
    return html.str();
}

int HtmlFromXml::ParseHeadingLevel(const XmlNode& node) const {
    std::string value;
    if (GetAttribute(node, "level", value)) {
        const int level = static_cast<int>(std::atof(value.c_str()));
        if (level >= 1 && level <= 6) {
            return level;
        }
    }
    const std::string& name = node.name;
    if (name.size() == 2 && (name[0] == 'H' || name[0] == 'h') && name[1] >= '1' && name[1] <= '6') {
        return name[1] - '0';
    }
    if (name.size() == 8 && (name.compare(0, 7, "Heading") == 0 || name.compare(0, 7, "heading") == 0) &&
        name[7] >= '1' && name[7] <= '6') {
        return name[7] - '0';
    }
    return 1;
}

std::string HtmlFromXml::BuildTextHtml(const XmlNode& node, const std::string& tagName) const {
    std::ostringstream html;
    html << "<" << tagName << " style=\"";

    if (!tagName.empty() && tagName[0] == 'h' && tagName.size() == 2 && tagName[1] >= '1' && tagName[1] <= '6') {
        const TextStyle defaults = Heading::DefaultStyle(tagName[1] - '0');
        html << "font-family:'Inter',sans-serif;"
             << "font-size:" << defaults.fontSize << "px;"
             << "font-weight:" << FontWeightToCss(defaults.fontWeight) << ";"
             << "line-height:" << defaults.lineHeight << ";"
             << "color:" << defaults.color.ToCss() << ";"
             << "margin:0 0 0.75rem 0;";
    } else if (tagName == "p") {
        const TextStyle defaults = Paragraph::DefaultStyle();
        html << "font-family:'Inter',sans-serif;"
             << "font-size:" << defaults.fontSize << "px;"
             << "font-weight:" << FontWeightToCss(defaults.fontWeight) << ";"
             << "line-height:" << defaults.lineHeight << ";"
             << "color:" << defaults.color.ToCss() << ";"
             << "max-width:" << defaults.maxWidth << ";"
             << "margin:0 0 0.75rem 0;";
    }

    html << StyleFromAttributes(node, true) << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
        } else if (child.name == "Span" || child.name == "span") {
            html << BuildSpanHtml(child);
        }
    }
    html << "</" << tagName << ">";
    return html.str();
}

std::string HtmlFromXml::BuildButtonHtml(const XmlNode& node) const {
    std::string label;
    if (!GetAttribute(node, "title", label)) {
        GetAttribute(node, "label", label);
    }
    if (label.empty()) {
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                label += child.text;
            }
        }
    }

    std::string variant = "filled";
    if (!GetAttribute(node, "variant", variant)) {
        GetAttribute(node, "style", variant);
    }

    std::string background = "#007AFF";
    std::string color = "#FFFFFF";
    if (variant == "tinted") {
        background = "#E5F1FF";
        color = "#007AFF";
    } else if (variant == "gray" || variant == "grey") {
        background = "#E5E5EA";
        color = "#1C1C1E";
    } else if (variant == "plain") {
        background = "transparent";
        color = "#007AFF";
    }

    std::string custom;
    if (GetAttribute(node, "background", custom) || GetAttribute(node, "backgroundColor", custom)) {
        background = custom;
    }
    if (GetAttribute(node, "color", custom)) {
        color = custom;
    }

    std::string align = "left";
    GetAttribute(node, "align", align);
    std::string fontSize = "17";
    GetAttribute(node, "fontSize", fontSize);
    std::string cornerRadius = "14";
    GetAttribute(node, "cornerRadius", cornerRadius);

    std::ostringstream html;
    html << "<button type=\"button\" style=\""
         << "display:inline-block;"
         << "font-family:'Inter',sans-serif;"
         << "font-size:" << fontSize << "px;"
         << "font-weight:600;"
         << "line-height:1.2;"
         << "white-space:nowrap;"
         << "border:none;"
         << "border-radius:" << cornerRadius << "px;"
         << "padding:12px 20px;"
         << "background:" << background << ";"
         << "color:" << color << ";"
         << "margin:0 0 16px 0;";
    if (align == "center") {
        html << "display:block;margin-left:auto;margin-right:auto;";
    } else if (align == "right") {
        html << "display:block;margin-left:auto;";
    }
    html << "\">" << EscapeHtml(label) << "</button>";
    return html.str();
}

std::string HtmlFromXml::BuildInputHtml(const XmlNode& node) const {
    std::string placeholder;
    GetAttribute(node, "placeholder", placeholder);
    if (placeholder.empty()) {
        GetAttribute(node, "hint", placeholder);
    }

    std::string value;
    if (!GetAttribute(node, "value", value)) {
        GetAttribute(node, "text", value);
    }
    if (value.empty()) {
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                value += child.text;
            }
        }
    }

    std::string variant = "filled";
    if (!GetAttribute(node, "variant", variant)) {
        GetAttribute(node, "style", variant);
    }

    std::string background = "#F2F2F7";
    std::string border = "none";
    std::string radius = "10";
    if (variant == "bordered" || variant == "border") {
        background = "#FFFFFF";
        border = "1px solid #C6C6C8";
        radius = "8";
    } else if (variant == "plain") {
        background = "transparent";
        border = "none";
        radius = "0";
    }

    std::string custom;
    if (GetAttribute(node, "background", custom) || GetAttribute(node, "backgroundColor", custom)) {
        background = custom;
    }
    if (GetAttribute(node, "cornerRadius", custom)) {
        radius = custom;
    }

    std::string align = "left";
    GetAttribute(node, "align", align);
    std::string fontSize = "17";
    GetAttribute(node, "fontSize", fontSize);
    std::string width = "100%";
    GetAttribute(node, "width", width);

    std::ostringstream html;
    html << "<input type=\"text\" placeholder=\"" << EscapeHtml(placeholder) << "\" value=\""
         << EscapeHtml(value) << "\" style=\""
         << "box-sizing:border-box;"
         << "width:" << width << ";"
         << "font-family:'Inter',sans-serif;"
         << "font-size:" << fontSize << "px;"
         << "font-weight:400;"
         << "line-height:1.2;"
         << "color:#1C1C1E;"
         << "background:" << background << ";";
    if (variant == "plain") {
        html << "border:none;border-bottom:1px solid #C6C6C8;border-radius:0;";
    } else {
        html << "border:" << border << ";"
             << "border-radius:" << radius << "px;";
    }
    html << "padding:10px 12px;"
         << "outline-color:#007AFF;"
         << "margin:0 0 16px 0;";
    if (align == "center") {
        html << "display:block;margin-left:auto;margin-right:auto;";
    } else if (align == "right") {
        html << "display:block;margin-left:auto;";
    }
    html << "\" />";
    return html.str();
}

std::string HtmlFromXml::BuildScrollbarHtml(const XmlNode& node) const {
    std::string orientation = "vertical";
    if (!GetAttribute(node, "orientation", orientation)) {
        GetAttribute(node, "direction", orientation);
    }
    const bool vertical = !(orientation == "horizontal" || orientation == "h");

    std::string attr;
    std::string thickness = "8";
    if (!GetAttribute(node, "thickness", thickness)) {
        GetAttribute(node, "size", thickness);
    }
    std::string length = "220";
    if (!GetAttribute(node, "length", length)) {
        GetAttribute(node, "trackLength", length);
    }
    std::string radius = "4";
    GetAttribute(node, "cornerRadius", radius);

    std::string trackColor = "transparent";
    if (GetAttribute(node, "trackColor", attr)) {
        trackColor = Color::FromHex(attr).ToCss();
    }

    std::string thumbColorHex = "#C7C7CC";
    if (!GetAttribute(node, "thumbColor", attr)) {
        GetAttribute(node, "color", attr);
    }
    if (!attr.empty()) {
        thumbColorHex = attr;
    }
    const std::string thumbColor = Color::FromHex(thumbColorHex).ToCss();

    double ratio = 1.0;
    std::string ratioStr;
    if (GetAttribute(node, "thumbRatio", ratioStr) || GetAttribute(node, "thumbSize", ratioStr)) {
        ratio = std::atof(ratioStr.c_str());
    } else {
        std::string content;
        std::string viewport;
        if (GetAttribute(node, "contentSize", content) && GetAttribute(node, "viewportSize", viewport)) {
            const double contentSize = std::atof(content.c_str());
            const double viewportSize = std::atof(viewport.c_str());
            if (contentSize > 0.0 && viewportSize > 0.0) {
                ratio = viewportSize / contentSize;
            }
        }
    }
    if (ratio < 0.05) {
        ratio = 0.05;
    } else if (ratio > 1.0) {
        ratio = 1.0;
    }

    double value = 0.0;
    if (GetAttribute(node, "value", attr)) {
        value = std::atof(attr.c_str());
    }
    if (value < 0.0) {
        value = 0.0;
    } else if (value > 1.0) {
        value = 1.0;
    }

    // Filled arrowheads at both ends of the track.
    std::string arrowsValue;
    bool showArrows = false;
    if (GetAttribute(node, "arrows", arrowsValue) || GetAttribute(node, "showArrows", arrowsValue) ||
        GetAttribute(node, "arrow", arrowsValue)) {
        showArrows = arrowsValue == "true" || arrowsValue == "1" || arrowsValue == "yes";
    }

    // Auto-hide: the control only appears while the pointer is over it.
    std::string autoHideValue;
    bool autoHide = false;
    if (GetAttribute(node, "autoHide", autoHideValue) || GetAttribute(node, "autohide", autoHideValue)) {
        autoHide = autoHideValue == "true" || autoHideValue == "1" || autoHideValue == "yes";
    } else {
        std::string variant;
        if (GetAttribute(node, "variant", variant)) {
            autoHide = variant == "thin" || variant == "overlay";
        }
    }

    // visibility=auto drops the element entirely while nothing overflows, so the
    // static HTML never renders a pointless scrollbar.
    std::string visibilityValue;
    if (GetAttribute(node, "visibility", visibilityValue) || GetAttribute(node, "showWhen", visibilityValue)) {
        if (visibilityValue == "auto" || visibilityValue == "overflow") {
            std::string contentValue;
            std::string viewportValue;
            const double contentSize = GetAttribute(node, "contentSize", contentValue) ||
                                               GetAttribute(node, "content", contentValue)
                                           ? std::atof(contentValue.c_str())
                                           : 0.0;
            const double viewportSize = GetAttribute(node, "viewportSize", viewportValue) ||
                                                GetAttribute(node, "viewport", viewportValue) ||
                                                GetAttribute(node, "pageSize", viewportValue)
                                            ? std::atof(viewportValue.c_str())
                                            : 0.0;
            const bool overflowing = (contentSize > 0.0 && viewportSize > 0.0)
                                         ? contentSize > viewportSize
                                         : ratio < 0.999999;
            if (!overflowing) {
                return std::string();
            }
        }
    }

    const double arrowSize = showArrows ? std::max(std::atof(thickness.c_str()), 6.0) : 0.0;
    const double arrowCap = showArrows ? arrowSize + 2.0 : 0.0;
    const double trackLength = std::atof(length.c_str());
    const double innerLength = std::max(0.0, trackLength - arrowCap * 2.0);
    const double thumbLength = std::max(18.0, ratio * innerLength);
    const double thumbStart = arrowCap + std::max(0.0, innerLength - thumbLength) * value;

    const double offsetPercent = (1.0 - ratio) * value * 100.0;
    const double thumbPercent = ratio * 100.0;

    std::ostringstream html;
    html << "<div role=\"scrollbar\" aria-orientation=\"" << (vertical ? "vertical" : "horizontal")
         << "\" class=\"RoboScrollbar\"";
    if (autoHide) {
        html << " data-autohide=\"true\"";
    }
    html << " style=\"border-radius:" << radius
         << "px;background:" << trackColor << ";margin:0 0 16px 0;";
    if (vertical) {
        html << "width:" << thickness << "px;height:" << length << "px;";
    } else {
        html << "width:" << length << "px;height:" << thickness << "px;";
    }
    html << "\">";

    if (showArrows) {
        std::string arrowColorValue;
        if (GetAttribute(node, "arrowColor", arrowColorValue)) {
            arrowColorValue = Color::FromHex(arrowColorValue).ToCss();
        } else {
            arrowColorValue = "#8E8E93";
        }
        if (vertical) {
            html << "<span class=\"RoboScrollbarArrow\" style=\"--RoboArrow:" << arrowColorValue
                 << ";left:0;right:0;top:0;height:" << arrowSize
                 << "px;clip-path:polygon(50% 24%,24% 76%,76% 76%);\"></span>";
            html << "<span class=\"RoboScrollbarArrow\" style=\"--RoboArrow:" << arrowColorValue
                 << ";left:0;right:0;bottom:0;height:" << arrowSize
                 << "px;clip-path:polygon(50% 76%,24% 24%,76% 24%);\"></span>";
        } else {
            html << "<span class=\"RoboScrollbarArrow\" style=\"--RoboArrow:" << arrowColorValue
                 << ";top:0;bottom:0;left:0;width:" << arrowSize
                 << "px;clip-path:polygon(24% 50%,76% 24%,76% 76%);\"></span>";
            html << "<span class=\"RoboScrollbarArrow\" style=\"--RoboArrow:" << arrowColorValue
                 << ";top:0;bottom:0;right:0;width:" << arrowSize
                 << "px;clip-path:polygon(76% 50%,24% 24%,24% 76%);\"></span>";
        }
    }

    html << "<div style=\"position:absolute;border-radius:" << radius << "px;background:" << thumbColor << ";";
    if (showArrows) {
        if (vertical) {
            html << "left:0;right:0;top:" << thumbStart << "px;height:" << thumbLength << "px;";
        } else {
            html << "top:0;bottom:0;left:" << thumbStart << "px;width:" << thumbLength << "px;";
        }
    } else if (vertical) {
        html << "left:0;right:0;top:" << offsetPercent << "%;height:" << thumbPercent << "%;";
    } else {
        html << "top:0;bottom:0;left:" << offsetPercent << "%;width:" << thumbPercent << "%;";
    }
    html << "\"></div></div>";
    return html.str();
}

// A container becomes a clipped flex region, so the static HTML keeps the same
// child order, spacing and stacking as the canvas without any scripting.
std::string HtmlFromXml::BuildContainerHtml(const XmlNode& node) const {
    std::string value;
    std::string layout = "stack";
    GetAttribute(node, "layout", layout);
    bool horizontal = false;
    if (GetAttribute(node, "orientation", value) || GetAttribute(node, "scrollAxis", value) ||
        GetAttribute(node, "axis", value) || (layout == "stack" && GetAttribute(node, "direction", value))) {
        horizontal = value == "horizontal" || value == "h" || value == "row" || value == "x";
    }

    std::ostringstream style;
    style << "box-sizing:border-box;overflow:hidden;";
    if (layout == "flex" || layout == "flexbox") {
        style << "display:flex;";
        std::string direction = "row";
        if (!GetAttribute(node, "flexDirection", direction)) {
            GetAttribute(node, "direction", direction);
        }
        style << "flex-direction:" << direction << ";";
        if (GetAttribute(node, "justify", value) || GetAttribute(node, "justifyContent", value)) {
            style << "justify-content:" << value << ";";
        }
        if (GetAttribute(node, "align", value) || GetAttribute(node, "alignItems", value)) {
            style << "align-items:" << value << ";";
        }
        if (GetAttribute(node, "wrap", value) || GetAttribute(node, "flexWrap", value)) {
            style << "flex-wrap:" << value << ";";
        }
    } else if (layout == "grid") {
        style << "display:grid;";
        if (GetAttribute(node, "columns", value) || GetAttribute(node, "templateColumns", value)) {
            style << "grid-template-columns:" << value << ";";
        }
        if (GetAttribute(node, "rows", value) || GetAttribute(node, "templateRows", value)) {
            style << "grid-template-rows:" << value << ";";
        }
        if (GetAttribute(node, "justify", value) || GetAttribute(node, "justifyItems", value)) {
            style << "justify-items:" << value << ";";
        }
        if (GetAttribute(node, "align", value) || GetAttribute(node, "alignItems", value)) {
            style << "align-items:" << value << ";";
        }
    } else if (layout == "position" || layout == "absolute") {
        style << "display:block;position:relative;";
    } else {
        style << "display:flex;";
        style << (horizontal ? "flex-direction:row;align-items:stretch;" : "flex-direction:column;");
    }
    if (horizontal) {
        style << "max-width:100%;";
    }

    if (GetAttribute(node, "width", value)) {
        style << "width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "height", value)) {
        style << "height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minWidth", value)) {
        style << "min-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style << "max-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minHeight", value)) {
        style << "min-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxHeight", value)) {
        style << "max-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "padding", value)) {
        style << "padding:" << value << "px;";
    }
    if (GetAttribute(node, "paddingX", value)) {
        style << "padding-left:" << value << "px;padding-right:" << value << "px;";
    }
    if (GetAttribute(node, "paddingY", value)) {
        style << "padding-top:" << value << "px;padding-bottom:" << value << "px;";
    }
    if (GetAttribute(node, "paddingTop", value)) {
        style << "padding-top:" << value << "px;";
    }
    if (GetAttribute(node, "paddingBottom", value)) {
        style << "padding-bottom:" << value << "px;";
    }
    if (GetAttribute(node, "paddingLeft", value)) {
        style << "padding-left:" << value << "px;";
    }
    if (GetAttribute(node, "paddingRight", value)) {
        style << "padding-right:" << value << "px;";
    }
    if (GetAttribute(node, "gap", value) || GetAttribute(node, "itemGap", value)) {
        style << "gap:" << value << "px;";
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style << "background:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderColor", value)) {
        style << "border-color:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderWidth", value)) {
        style << "border-width:" << value << "px;border-style:solid;";
    }
    if (GetAttribute(node, "cornerRadius", value) || GetAttribute(node, "borderRadius", value)) {
        style << "border-radius:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    std::ostringstream html;
    html << "<section class=\"RoboContainer\" style=\"" << style.str() << "\"";
    html << (horizontal ? " data-orientation=\"horizontal\"" : " data-orientation=\"vertical\"");
    html << " data-layout=\"" << layout << "\"";
    html << ">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            continue;
        }
        html << ConvertNode(child);
    }
    html << "</section>";
    return html.str();
}

std::string HtmlFromXml::ConvertNode(const XmlNode& node) const {
    if (node.isText) {
        return EscapeHtml(node.text);
    }

    if (node.name == "Robo") {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
             << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
             << "<title>Robo Page</title>"
             << "<style>@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Regular.ttf');font-weight:400;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Medium.ttf');font-weight:500;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-SemiBold.ttf');font-weight:600;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Bold.ttf');font-weight:700;}"
             << "@font-face{font-family:'Inter';src:url('/Assets/Fonts/Inter-Italic.ttf');font-style:italic;font-weight:400;}"
             << "body{margin:0;padding:24px;background:#FFFFFF;color:#1A1A1A;font-family:'Inter',sans-serif;}"
             << ".RoboContainer{box-sizing:border-box;overflow:hidden;}"
             << ".RoboContainer[data-orientation='vertical']>*{flex:0 0 auto;align-self:stretch;}"
             << ".RoboContainer[data-orientation='horizontal']>*{flex:0 0 auto;}"
             << ".RoboScrollbar{position:relative;overflow:hidden;box-sizing:border-box;cursor:default;}"
             << ".RoboScrollbar[data-autohide='true']{opacity:0;transition:opacity .18s ease;}"
             << ".RoboScrollbar[data-autohide='true']:hover,.RoboScrollbar[data-autohide='true']:focus-within{opacity:1;}"
             << ".RoboScrollbarArrow{position:absolute;display:block;pointer-events:none;"
                "background:var(--RoboArrow,#8E8E93);}"
             << ".RoboScrollbar:hover .RoboScrollbarArrow{background:#636366;}"
             << ".RoboScrollbar:active .RoboScrollbarArrow{background:#1C1C1E;}"
             << ".RoboScrollbar:active>div{filter:brightness(0.92);}"
             << "</style>"
             << "</head><body>";
        for (const XmlNode& child : node.children) {
            html << ConvertNode(child);
        }
        html << "</body></html>";
        return html.str();
    }

    if (node.name == "Text") {
        return BuildTextHtml(node, "p");
    }

    if (node.name == "Heading" || node.name == "heading" ||
        (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
         node.name[1] <= '6') ||
        (node.name.size() == 8 && (node.name.compare(0, 7, "Heading") == 0 || node.name.compare(0, 7, "heading") == 0) &&
         node.name[7] >= '1' && node.name[7] <= '6')) {
        return BuildTextHtml(node, "h" + std::to_string(ParseHeadingLevel(node)));
    }

    if (node.name == "Paragraph" || node.name == "paragraph" || node.name == "P" || node.name == "p") {
        return BuildTextHtml(node, "p");
    }

    if (node.name == "Button") {
        return BuildButtonHtml(node);
    }

    if (node.name == "Input" || node.name == "TextField" || node.name == "TextInput") {
        return BuildInputHtml(node);
    }

    if (node.name == "Scrollbar" || node.name == "scrollbar") {
        return BuildScrollbarHtml(node);
    }

    if (node.name == "Container" || node.name == "container") {
        return BuildContainerHtml(node);
    }

    if (node.name == "Flexbox" || node.name == "flexbox" || node.name == "Flex" || node.name == "flex") {
        return BuildFlexboxHtml(node);
    }

    if (node.name == "Grid" || node.name == "grid") {
        return BuildGridHtml(node);
    }

    if (node.name == "Position" || node.name == "position" || node.name == "Positioned" || node.name == "positioned") {
        return BuildPositionHtml(node);
    }

    return "";
}

std::string HtmlFromXml::BuildFlexboxHtml(const XmlNode& node) const {
    std::ostringstream style;
    style << "display:flex;box-sizing:border-box;";

    std::string value;
    if (GetAttribute(node, "direction", value) || GetAttribute(node, "flexDirection", value)) {
        if (value == "column" || value == "col") {
            style << "flex-direction:column;";
        } else if (value == "column-reverse" || value == "col-reverse") {
            style << "flex-direction:column-reverse;";
        } else if (value == "row-reverse") {
            style << "flex-direction:row-reverse;";
        } else {
            style << "flex-direction:row;";
        }
    } else {
        style << "flex-direction:row;";
    }

    if (GetAttribute(node, "justify", value) || GetAttribute(node, "justifyContent", value)) {
        if (value == "center") {
            style << "justify-content:center;";
        } else if (value == "flex-end" || value == "end") {
            style << "justify-content:flex-end;";
        } else if (value == "space-between" || value == "between") {
            style << "justify-content:space-between;";
        } else if (value == "space-around" || value == "around") {
            style << "justify-content:space-around;";
        } else if (value == "space-evenly" || value == "evenly") {
            style << "justify-content:space-evenly;";
        } else {
            style << "justify-content:flex-start;";
        }
    }
    if (GetAttribute(node, "align", value) || GetAttribute(node, "alignItems", value)) {
        if (value == "flex-start" || value == "start") {
            style << "align-items:flex-start;";
        } else if (value == "center") {
            style << "align-items:center;";
        } else if (value == "flex-end" || value == "end") {
            style << "align-items:flex-end;";
        } else {
            style << "align-items:stretch;";
        }
    }
    if (GetAttribute(node, "wrap", value) || GetAttribute(node, "flexWrap", value)) {
        if (value == "wrap") {
            style << "flex-wrap:wrap;";
        } else if (value == "wrap-reverse") {
            style << "flex-wrap:wrap-reverse;";
        } else {
            style << "flex-wrap:nowrap;";
        }
    }
    if (GetAttribute(node, "gap", value)) {
        style << "gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapX", value) || GetAttribute(node, "columnGap", value)) {
        style << "column-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapY", value) || GetAttribute(node, "rowGap", value)) {
        style << "row-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "width", value)) {
        style << "width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "height", value)) {
        style << "height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minWidth", value)) {
        style << "min-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style << "max-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minHeight", value)) {
        style << "min-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxHeight", value)) {
        style << "max-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "padding", value)) {
        style << "padding:" << value << "px;";
    }
    if (GetAttribute(node, "paddingX", value)) {
        style << "padding-left:" << value << "px;padding-right:" << value << "px;";
    }
    if (GetAttribute(node, "paddingY", value)) {
        style << "padding-top:" << value << "px;padding-bottom:" << value << "px;";
    }
    if (GetAttribute(node, "margin", value)) {
        style << "margin:" << value << "px;";
    } else {
        style << "margin:0 0 16px 0;";
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style << "background:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderColor", value)) {
        style << "border:1px solid " << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderWidth", value)) {
        style << "border-width:" << value << "px;";
    }
    if (GetAttribute(node, "cornerRadius", value) || GetAttribute(node, "borderRadius", value)) {
        style << "border-radius:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    std::ostringstream html;
    html << "<div style=\"" << style.str() << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
            continue;
        }
        std::string flex;
        std::string alignSelf;
        std::string childWidth;
        std::string childHeight;
        std::string childMinWidth;
        std::string childMaxWidth;
        std::string childMinHeight;
        std::string childMaxHeight;
        const bool hasFlex = GetAttribute(child, "flex", flex) || GetAttribute(child, "flexGrow", flex);
        const bool hasAlign = GetAttribute(child, "alignSelf", alignSelf);
        const bool hasW = GetAttribute(child, "width", childWidth);
        const bool hasH = GetAttribute(child, "height", childHeight);
        const bool hasMinW = GetAttribute(child, "minWidth", childMinWidth);
        const bool hasMaxW = GetAttribute(child, "maxWidth", childMaxWidth);
        const bool hasMinH = GetAttribute(child, "minHeight", childMinHeight);
        const bool hasMaxH = GetAttribute(child, "maxHeight", childMaxHeight);
        if (hasFlex || hasAlign || hasW || hasH || hasMinW || hasMaxW || hasMinH || hasMaxH) {
            std::ostringstream wrapStyle;
            wrapStyle << "box-sizing:border-box;";
            if (hasFlex) {
                wrapStyle << "flex:" << flex << ";";
            }
            if (hasAlign) {
                wrapStyle << "align-self:" << alignSelf << ";";
            }
            if (hasW) {
                wrapStyle << "width:" << FormatDimensionCss(childWidth) << ";";
            }
            if (hasH) {
                wrapStyle << "height:" << FormatDimensionCss(childHeight) << ";";
            }
            if (hasMinW) {
                wrapStyle << "min-width:" << FormatDimensionCss(childMinWidth) << ";";
            }
            if (hasMaxW) {
                wrapStyle << "max-width:" << FormatDimensionCss(childMaxWidth) << ";";
            }
            if (hasMinH) {
                wrapStyle << "min-height:" << FormatDimensionCss(childMinHeight) << ";";
            }
            if (hasMaxH) {
                wrapStyle << "max-height:" << FormatDimensionCss(childMaxHeight) << ";";
            }
            html << "<div style=\"" << wrapStyle.str() << "\">" << ConvertNode(child) << "</div>";
        } else {
            html << ConvertNode(child);
        }
    }
    html << "</div>";
    return html.str();
}

std::string HtmlFromXml::BuildGridHtml(const XmlNode& node) const {
    std::ostringstream style;
    style << "display:grid;box-sizing:border-box;";

    std::string value;
    if (GetAttribute(node, "columns", value) || GetAttribute(node, "templateColumns", value) ||
        GetAttribute(node, "gridTemplateColumns", value)) {
        bool isNumber = true;
        for (char c : value) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                isNumber = false;
                break;
            }
        }
        if (isNumber && !value.empty()) {
            style << "grid-template-columns:repeat(" << value << ", 1fr);";
        } else {
            style << "grid-template-columns:" << value << ";";
        }
    } else if (GetAttribute(node, "columnCount", value)) {
        style << "grid-template-columns:repeat(" << value << ", 1fr);";
    }

    if (GetAttribute(node, "rows", value) || GetAttribute(node, "templateRows", value) ||
        GetAttribute(node, "gridTemplateRows", value)) {
        style << "grid-template-rows:" << value << ";";
    }

    if (GetAttribute(node, "gap", value)) {
        style << "gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapX", value) || GetAttribute(node, "columnGap", value)) {
        style << "column-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "gapY", value) || GetAttribute(node, "rowGap", value)) {
        style << "row-gap:" << value << (value.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "autoFlow", value)) {
        style << "grid-auto-flow:" << value << ";";
    }
    if (GetAttribute(node, "justifyItems", value) || GetAttribute(node, "justify", value)) {
        style << "justify-items:" << value << ";";
    }
    if (GetAttribute(node, "alignItems", value) || GetAttribute(node, "align", value)) {
        style << "align-items:" << value << ";";
    }
    if (GetAttribute(node, "width", value)) {
        style << "width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "height", value)) {
        style << "height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minWidth", value)) {
        style << "min-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxWidth", value)) {
        style << "max-width:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "minHeight", value)) {
        style << "min-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "maxHeight", value)) {
        style << "max-height:" << FormatDimensionCss(value) << ";";
    }
    if (GetAttribute(node, "padding", value)) {
        style << "padding:" << value << "px;";
    }
    if (GetAttribute(node, "paddingX", value)) {
        style << "padding-left:" << value << "px;padding-right:" << value << "px;";
    }
    if (GetAttribute(node, "paddingY", value)) {
        style << "padding-top:" << value << "px;padding-bottom:" << value << "px;";
    }
    if (GetAttribute(node, "margin", value)) {
        style << "margin:" << value << "px;";
    } else {
        style << "margin:0 0 16px 0;";
    }
    if (GetAttribute(node, "background", value) || GetAttribute(node, "backgroundColor", value)) {
        style << "background:" << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderColor", value)) {
        style << "border:1px solid " << Color::FromHex(value).ToCss() << ";";
    }
    if (GetAttribute(node, "borderWidth", value)) {
        style << "border-width:" << value << "px;";
    }
    if (GetAttribute(node, "cornerRadius", value) || GetAttribute(node, "borderRadius", value)) {
        style << "border-radius:" << value << "px;";
    }
    if (GetAttribute(node, "opacity", value)) {
        style << "opacity:" << value << ";";
    }

    std::ostringstream html;
    html << "<div style=\"" << style.str() << "\">";
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
            continue;
        }

        std::string col;
        std::string row;
        std::string colSpan;
        std::string rowSpan;
        std::string justifySelf;
        std::string alignSelf;
        std::string childWidth;
        std::string childHeight;
        std::string childMinWidth;
        std::string childMaxWidth;
        std::string childMinHeight;
        std::string childMaxHeight;
        const bool hasCol = GetAttribute(child, "column", col) || GetAttribute(child, "gridColumn", col) || GetAttribute(child, "col", col);
        const bool hasRow = GetAttribute(child, "row", row) || GetAttribute(child, "gridRow", row);
        const bool hasColSpan = GetAttribute(child, "columnSpan", colSpan) || GetAttribute(child, "colSpan", colSpan);
        const bool hasRowSpan = GetAttribute(child, "rowSpan", rowSpan);
        const bool hasJustify = GetAttribute(child, "justifySelf", justifySelf);
        const bool hasAlign = GetAttribute(child, "alignSelf", alignSelf);
        const bool hasW = GetAttribute(child, "width", childWidth);
        const bool hasH = GetAttribute(child, "height", childHeight);
        const bool hasMinW = GetAttribute(child, "minWidth", childMinWidth);
        const bool hasMaxW = GetAttribute(child, "maxWidth", childMaxWidth);
        const bool hasMinH = GetAttribute(child, "minHeight", childMinHeight);
        const bool hasMaxH = GetAttribute(child, "maxHeight", childMaxHeight);

        if (hasCol || hasRow || hasColSpan || hasRowSpan || hasJustify || hasAlign || hasW || hasH || hasMinW || hasMaxW || hasMinH || hasMaxH) {
            std::ostringstream itemStyle;
            itemStyle << "box-sizing:border-box;";
            if (hasColSpan && hasCol) {
                itemStyle << "grid-column:" << col << " / span " << colSpan << ";";
            } else if (hasColSpan) {
                itemStyle << "grid-column:span " << colSpan << ";";
            } else if (hasCol) {
                itemStyle << "grid-column:" << col << ";";
            }

            if (hasRowSpan && hasRow) {
                itemStyle << "grid-row:" << row << " / span " << rowSpan << ";";
            } else if (hasRowSpan) {
                itemStyle << "grid-row:span " << rowSpan << ";";
            } else if (hasRow) {
                itemStyle << "grid-row:" << row << ";";
            }

            if (hasJustify) {
                itemStyle << "justify-self:" << justifySelf << ";";
            }
            if (hasAlign) {
                itemStyle << "align-self:" << alignSelf << ";";
            }
            if (hasW) {
                itemStyle << "width:" << FormatDimensionCss(childWidth) << ";";
            }
            if (hasH) {
                itemStyle << "height:" << FormatDimensionCss(childHeight) << ";";
            }
            if (hasMinW) {
                itemStyle << "min-width:" << FormatDimensionCss(childMinWidth) << ";";
            }
            if (hasMaxW) {
                itemStyle << "max-width:" << FormatDimensionCss(childMaxWidth) << ";";
            }
            if (hasMinH) {
                itemStyle << "min-height:" << FormatDimensionCss(childMinHeight) << ";";
            }
            if (hasMaxH) {
                itemStyle << "max-height:" << FormatDimensionCss(childMaxHeight) << ";";
            }
            html << "<div style=\"" << itemStyle.str() << "\">" << ConvertNode(child) << "</div>";
        } else {
            html << ConvertNode(child);
        }
    }
    html << "</div>";
    return html.str();
}

std::string HtmlFromXml::BuildPositionHtml(const XmlNode& node) const {
    std::ostringstream style;
    std::string mode = "relative";
    std::string attr;
    if (GetAttribute(node, "mode", attr) || GetAttribute(node, "position", attr)) {
        mode = attr;
    }
    style << "position:" << mode << ";box-sizing:border-box;";

    if (GetAttribute(node, "left", attr) || GetAttribute(node, "x", attr)) {
        style << "left:" << attr << (attr.find('%') == std::string::npos && attr.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "top", attr) || GetAttribute(node, "y", attr)) {
        style << "top:" << attr << (attr.find('%') == std::string::npos && attr.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "right", attr)) {
        style << "right:" << attr << (attr.find('%') == std::string::npos && attr.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "bottom", attr)) {
        style << "bottom:" << attr << (attr.find('%') == std::string::npos && attr.find("px") == std::string::npos ? "px;" : ";");
    }
    if (GetAttribute(node, "width", attr)) {
        style << "width:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "height", attr)) {
        style << "height:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "minWidth", attr)) {
        style << "min-width:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "maxWidth", attr)) {
        style << "max-width:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "minHeight", attr)) {
        style << "min-height:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "maxHeight", attr)) {
        style << "max-height:" << FormatDimensionCss(attr) << ";";
    }
    if (GetAttribute(node, "zIndex", attr)) {
        style << "z-index:" << attr << ";";
    }
    if (GetAttribute(node, "padding", attr)) {
        style << "padding:" << attr << "px;";
    }
    if (GetAttribute(node, "background", attr) || GetAttribute(node, "backgroundColor", attr)) {
        style << "background:" << attr << ";";
    }
    if (GetAttribute(node, "borderColor", attr)) {
        style << "border:" << (GetAttribute(node, "borderWidth", attr) ? attr : "1") << "px solid " << attr << ";";
    }
    if (GetAttribute(node, "cornerRadius", attr) || GetAttribute(node, "borderRadius", attr)) {
        style << "border-radius:" << attr << "px;";
    }

    std::ostringstream html;
    html << "<div style=\"" << style.str() << "\">";

    for (const XmlNode& child : node.children) {
        if (child.isText) {
            html << EscapeHtml(child.text);
            continue;
        }

        std::ostringstream itemStyle;
        itemStyle << "position:absolute;box-sizing:border-box;";
        if (GetAttribute(child, "left", attr) || GetAttribute(child, "x", attr)) {
            itemStyle << "left:" << (attr.find('%') != std::string::npos || attr.find("px") != std::string::npos ? attr : attr + "px") << ";";
        }
        if (GetAttribute(child, "top", attr) || GetAttribute(child, "y", attr)) {
            itemStyle << "top:" << (attr.find('%') != std::string::npos || attr.find("px") != std::string::npos ? attr : attr + "px") << ";";
        }
        if (GetAttribute(child, "right", attr)) {
            itemStyle << "right:" << (attr.find('%') != std::string::npos || attr.find("px") != std::string::npos ? attr : attr + "px") << ";";
        }
        if (GetAttribute(child, "bottom", attr)) {
            itemStyle << "bottom:" << (attr.find('%') != std::string::npos || attr.find("px") != std::string::npos ? attr : attr + "px") << ";";
        }
        if (GetAttribute(child, "zIndex", attr)) {
            itemStyle << "z-index:" << attr << ";";
        }
        if (GetAttribute(child, "width", attr)) {
            itemStyle << "width:" << FormatDimensionCss(attr) << ";";
        }
        if (GetAttribute(child, "height", attr)) {
            itemStyle << "height:" << FormatDimensionCss(attr) << ";";
        }
        if (GetAttribute(child, "minWidth", attr)) {
            itemStyle << "min-width:" << FormatDimensionCss(attr) << ";";
        }
        if (GetAttribute(child, "maxWidth", attr)) {
            itemStyle << "max-width:" << FormatDimensionCss(attr) << ";";
        }
        if (GetAttribute(child, "minHeight", attr)) {
            itemStyle << "min-height:" << FormatDimensionCss(attr) << ";";
        }
        if (GetAttribute(child, "maxHeight", attr)) {
            itemStyle << "max-height:" << FormatDimensionCss(attr) << ";";
        }

        html << "<div style=\"" << itemStyle.str() << "\">" << ConvertNode(child) << "</div>";
    }

    html << "</div>";
    return html.str();
}

std::string HtmlFromXml::Convert(const std::string& xml) const {
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    return ConvertNode(root);
}

}  // namespace Robo
