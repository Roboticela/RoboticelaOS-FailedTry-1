from pathlib import Path

path = Path(r"d:\NEW\RoboticelaOS\Compiler\XMLCompiler\Source\XmlToCpp.cpp")
src = path.read_text(encoding="utf-8")

# Insert BindPropsLength before BindWeight if missing
if "BindPropsLength" not in src:
    needle = 'std::string XmlToCpp::BindWeight(int indent, const std::string& object, const std::string& value, const std::string& element) const {'
    insert = r'''std::string XmlToCpp::BindPropsLength(int indent, const std::string& object, const std::string& field,
                                      const std::string& value) const {
    std::string lengthExpr = "Robo::Length::Auto()";
    if (value == "auto" || value.empty()) {
        lengthExpr = "Robo::Length::Auto()";
    } else if (!value.empty() && value.back() == '%') {
        lengthExpr = "Robo::Length::Percent(" + FloatLiteral(value.substr(0, value.size() - 1)) + ")";
    } else if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        lengthExpr = "Robo::Length::Rem(" + FloatLiteral(value.substr(0, value.size() - 3)) + ")";
    } else if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value.substr(0, value.size() - 2)) + ")";
    } else {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value) + ")";
    }
    const std::string lengthField = field + "Length";
    const std::string hasField = "has" + std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(field[0])))) +
                                 field.substr(1) + "Length";
    return Indent(indent) + object + "." + field + " = \"" + EscapeCpp(value) + "\";\n" + Indent(indent) + object +
           "." + lengthField + " = " + lengthExpr + ";\n" + Indent(indent) + object + "." + hasField + " = true;\n";
}

'''
    if needle not in src:
        raise SystemExit("BindWeight not found")
    src = src.replace(needle, insert + needle, 1)

old = '''code << Indent(indent + 1) << "props.width = \\"" << EscapeCpp(attribute.second) << "\\";\\n";'''
new = '''code << BindPropsLength(indent + 1, "props", "width", attribute.second);'''
# file uses real escapes differently - read a sample
idx = src.find('props.width = "')
print("idx", idx)
print(repr(src[idx-80:idx+120]) if idx >= 0 else "not found")
path.write_text(src, encoding="utf-8", newline="\n")
