#include "Robo/Foundation.hpp"

namespace Robo {

namespace {

Foundation BuildRoboFoundation() {
    Foundation foundation;
    foundation.SetName("RoboFoundation");

    auto addColor = [&](const char* name, const char* light, const char* dark) {
        foundation.SetColor(name, Color::FromHex(light), Color::FromHex(dark));
    };

    addColor("brand.primary", "#0E7AFF", "#3B9EFF");
    addColor("brand.primary.hover", "#0A6AE8", "#5CADFF");
    addColor("brand.primary.active", "#0757C7", "#2E8CE6");
    addColor("brand.primary.disabled", "#B8D9FF", "#1E3A5C");
    addColor("brand.primary.subtle", "#0E7AFF1A", "#3B9EFF26");
    addColor("brand.primary.range", "#0E7AFF38", "#3B9EFF61");
    addColor("brand.primary.focus-ring", "#0E7AFF40", "#3B9EFF4D");
    addColor("accent.teal", "#14B8A6", "#2DD4C4");
    addColor("accent.teal.subtle", "#14B8A61A", "#2DD4C426");
    addColor("success", "#30D158", "#30D158");
    addColor("success.subtle", "#30D1581A", "#30D15826");
    addColor("success.focus-ring", "#30D15840", "#30D1584D");
    addColor("warning", "#FF9F0A", "#FF9F0A");
    addColor("warning.subtle", "#FF9F0A1A", "#FF9F0A26");
    addColor("danger", "#FF3B30", "#FF453A");
    addColor("danger.hover", "#E0342A", "#FF5A50");
    addColor("danger.subtle", "#FF3B301A", "#FF453A1A");
    addColor("danger.focus-ring", "#FF3B3040", "#FF453A66");
    addColor("text.primary", "#1D1D1F", "#FFFFFF");
    addColor("text.secondary", "#6E6E73", "#A1A1A6");
    addColor("text.tertiary", "#86868B", "#98989D");
    addColor("text.disabled", "#C7C7CC", "#48484A");
    addColor("text.on-brand", "#FFFFFF", "#FFFFFF");
    addColor("bg.primary", "#FFFFFF", "#000000");
    addColor("bg.secondary", "#F5F5F7", "#1C1C1E");
    addColor("bg.tertiary", "#EDEDEF", "#2C2C2E");
    addColor("bg.elevated", "#FFFFFF", "#2C2C2E");
    addColor("bg.hover", "#F5F5F7", "#2C2C2E");
    addColor("bg.active", "#EDEDEF", "#3A3A3C");
    addColor("bg.overlay", "#00000080", "#000000B3");
    addColor("border.default", "#D2D2D7", "#38383A");
    addColor("border.subtle", "#E5E5E7", "#2C2C2E");
    addColor("border.strong", "#B8B8BD", "#48484A");
    addColor("border.danger", "#FF3B304D", "#FF453A4D");

    auto addType = [&](const char* role, float size, float mobile, int weight, float tracking, float leading) {
        foundation.SetType(role, size, mobile, weight, tracking, leading);
    };
    addType("display", 80.0f, 40.0f, 600, -0.015f, 1.05f);
    addType("h1", 56.0f, 32.0f, 600, -0.015f, 1.07f);
    addType("h2", 40.0f, 28.0f, 600, -0.01f, 1.1f);
    addType("h3", 28.0f, 21.0f, 600, -0.005f, 1.14f);
    addType("subhead", 24.0f, 19.0f, 400, 0.0f, 1.33f);
    addType("body", 17.0f, 17.0f, 400, 0.0f, 1.47f);
    addType("bodyLg", 19.0f, 17.0f, 400, 0.0f, 1.42f);
    addType("caption", 12.0f, 12.0f, 400, 0.0f, 1.33f);
    addType("button", 17.0f, 17.0f, 500, 0.0f, 1.0f);

    foundation.SetSpace("1", 4.0f);
    foundation.SetSpace("2", 8.0f);
    foundation.SetSpace("3", 12.0f);
    foundation.SetSpace("4", 16.0f);
    foundation.SetSpace("5", 24.0f);
    foundation.SetSpace("6", 32.0f);
    foundation.SetSpace("7", 48.0f);
    foundation.SetSpace("8", 64.0f);
    foundation.SetSpace("9", 96.0f);
    foundation.SetSpace("10", 128.0f);

    foundation.SetRadius("sm", 6.0f);
    foundation.SetRadius("md", 8.0f);
    foundation.SetRadius("lg", 12.0f);
    foundation.SetRadius("xl", 16.0f);
    foundation.SetRadius("2xl", 20.0f);
    foundation.SetRadius("pill", 980.0f);

    foundation.SetLayout("breakpoint.tablet", 734.0f);
    foundation.SetLayout("breakpoint.desktop", 1068.0f);
    foundation.SetLayout("padding.mobile", 22.0f);
    foundation.SetLayout("padding.tablet", 40.0f);
    foundation.SetLayout("padding.desktop", 80.0f);
    foundation.SetLayout("columns.mobile", 1.0f);
    foundation.SetLayout("columns.tablet", 12.0f);
    foundation.SetLayout("columns.desktop", 12.0f);
    foundation.SetLayout("gutter.mobile", 16.0f);
    foundation.SetLayout("gutter.tablet", 24.0f);
    foundation.SetLayout("gutter.desktop", 24.0f);
    foundation.SetLayout("width.copy", 692.0f);
    foundation.SetLayout("width.docs", 760.0f);
    foundation.SetLayout("width.wide", 900.0f);
    foundation.SetLayout("width.marketing", 980.0f);
    foundation.SetLayout("width.section", 1024.0f);

    foundation.Seal();
    return foundation;
}

struct RegisterRoboFoundation {
    RegisterRoboFoundation() {
        Foundation::Register(Foundation::RoboFoundation());
    }
};

const RegisterRoboFoundation kRegisterRoboFoundation;

}  // namespace

const Foundation& Foundation::RoboFoundation() {
    static const Foundation foundation = BuildRoboFoundation();
    return foundation;
}

}  // namespace Robo
