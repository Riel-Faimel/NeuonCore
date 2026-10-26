#pragma once

#include <array>
#include <cstddef>
#include <string_view>

// 极简 JSON 解析器，纯 constexpr：在编译期把 JSON 文本解析成固定容量的 DOM。
// 支持对象、数组、字符串（不含转义）、数字（不含指数）、true / false / null。
// 用法：inline constexpr auto doc = json::parse(text);
namespace json {

enum class Type {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
};

struct Node {
    Type type = Type::Null;
    double number = 0.0;
    bool boolean = false;
    std::string_view text;
    int first = -1;   // 第一个子节点
    int next = -1;    // 下一个兄弟
    int key = -1;     // 对象成员的键（字符串节点）
};

class Document;

struct Parser;

class Document {
public:
    static constexpr int MaxNodes = 512;

    constexpr Document() = default;

    constexpr const Node &root() const { return nodes_[0]; }
    constexpr const Node &node(int index) const { return nodes_[index]; }

    constexpr const Node *member(const Node &object, std::string_view name) const
    {
        if (object.type != Type::Object) {
            return nullptr;
        }
        for (int index = object.first; index >= 0; index = nodes_[index].next) {
            const int keyIndex = nodes_[index].key;
            if (keyIndex >= 0 && nodes_[keyIndex].text == name) {
                return &nodes_[index];
            }
        }
        return nullptr;
    }

    constexpr int size(const Node &value) const
    {
        int total = 0;
        for (int index = value.first; index >= 0; index = nodes_[index].next) {
            ++total;
        }
        return total;
    }

    constexpr const Node *at(const Node &value, int index) const
    {
        int current = 0;
        for (int i = value.first; i >= 0; i = nodes_[i].next, ++current) {
            if (current == index) {
                return &nodes_[i];
            }
        }
        return nullptr;
    }

    constexpr double number(const Node *value, double fallback = 0.0) const
    {
        return value != nullptr && value->type == Type::Number ? value->number : fallback;
    }

    constexpr int integer(const Node *value, int fallback = 0) const
    {
        if (value == nullptr || value->type != Type::Number) {
            return fallback;
        }
        const double rounded = value->number < 0.0 ? value->number - 0.5 : value->number + 0.5;
        return int(rounded);
    }

    constexpr std::string_view text(const Node *value) const
    {
        return value != nullptr && value->type == Type::String ? value->text
                                                               : std::string_view{};
    }

    constexpr bool boolean(const Node *value, bool fallback = false) const
    {
        return value != nullptr && value->type == Type::Bool ? value->boolean : fallback;
    }

private:
    friend struct Parser;

    constexpr int add(Type type)
    {
        if (count_ >= MaxNodes) {
            return -1;
        }
        nodes_[count_] = Node{};
        nodes_[count_].type = type;
        return count_++;
    }

    constexpr Node &nodeRef(int index) { return nodes_[index]; }

    std::array<Node, MaxNodes> nodes_{};
    int count_ = 0;
};

struct Parser {
    std::string_view text;
    std::size_t pos = 0;
    Document document;

    constexpr void skip()
    {
        while (pos < text.size()) {
            const char c = text[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
                continue;
            }
            break;
        }
    }

    constexpr bool eat(char expected)
    {
        skip();
        if (pos < text.size() && text[pos] == expected) {
            ++pos;
            return true;
        }
        return false;
    }

    constexpr int addBool(bool value)
    {
        const int index = document.add(Type::Bool);
        if (index >= 0) {
            document.nodeRef(index).boolean = value;
        }
        return index;
    }

    constexpr int parseString()
    {
        if (pos >= text.size() || text[pos] != '"') {
            return -1;
        }
        ++pos;
        const std::size_t start = pos;
        while (pos < text.size() && text[pos] != '"') {
            if (text[pos] == '\\') {
                return -1;   // 数据文件里不使用转义
            }
            ++pos;
        }
        if (pos >= text.size()) {
            return -1;
        }
        const std::string_view value = text.substr(start, pos - start);
        ++pos;
        const int index = document.add(Type::String);
        if (index >= 0) {
            document.nodeRef(index).text = value;
        }
        return index;
    }

    constexpr int parseNumber()
    {
        const bool negative = pos < text.size() && text[pos] == '-';
        if (negative) {
            ++pos;
        }
        bool digits = false;
        double value = 0.0;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
            value = value * 10.0 + double(text[pos] - '0');
            ++pos;
            digits = true;
        }
        if (pos < text.size() && text[pos] == '.') {
            ++pos;
            double factor = 0.1;
            while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
                value += double(text[pos] - '0') * factor;
                factor *= 0.1;
                ++pos;
                digits = true;
            }
        }
        if (!digits) {
            return -1;
        }
        const int index = document.add(Type::Number);
        if (index >= 0) {
            document.nodeRef(index).number = negative ? -value : value;
        }
        return index;
    }

    constexpr int parseArray(int depth)
    {
        ++pos;   // '['
        const int array = document.add(Type::Array);
        if (array < 0) {
            return -1;
        }
        skip();
        if (pos < text.size() && text[pos] == ']') {
            ++pos;
            return array;
        }
        int last = -1;
        while (true) {
            const int value = parseValue(depth + 1);
            if (value < 0) {
                return -1;
            }
            if (last < 0) {
                document.nodeRef(array).first = value;
            } else {
                document.nodeRef(last).next = value;
            }
            last = value;
            if (eat(',')) {
                continue;
            }
            if (eat(']')) {
                break;
            }
            return -1;
        }
        return array;
    }

    constexpr int parseObject(int depth)
    {
        ++pos;   // '{'
        const int object = document.add(Type::Object);
        if (object < 0) {
            return -1;
        }
        skip();
        if (pos < text.size() && text[pos] == '}') {
            ++pos;
            return object;
        }
        int last = -1;
        while (true) {
            skip();
            const int key = parseString();
            if (key < 0) {
                return -1;
            }
            if (!eat(':')) {
                return -1;
            }
            const int value = parseValue(depth + 1);
            if (value < 0) {
                return -1;
            }
            document.nodeRef(value).key = key;
            if (last < 0) {
                document.nodeRef(object).first = value;
            } else {
                document.nodeRef(last).next = value;
            }
            last = value;
            if (eat(',')) {
                continue;
            }
            if (eat('}')) {
                break;
            }
            return -1;
        }
        return object;
    }

    constexpr int parseValue(int depth)
    {
        skip();
        if (pos >= text.size() || depth > 32) {
            return -1;
        }
        switch (text[pos]) {
        case '{':
            return parseObject(depth);
        case '[':
            return parseArray(depth);
        case '"':
            return parseString();
        case 't':
            if (text.substr(pos, 4) == "true") {
                pos += 4;
                return addBool(true);
            }
            return -1;
        case 'f':
            if (text.substr(pos, 5) == "false") {
                pos += 5;
                return addBool(false);
            }
            return -1;
        case 'n':
            if (text.substr(pos, 4) == "null") {
                pos += 4;
                return document.add(Type::Null);
            }
            return -1;
        default:
            return parseNumber();
        }
    }
};

// 编译期入口：解析失败时返回空 Document，调用方在启动时用
// data::validationError() 显式检查并转到非零退出码。
constexpr Document parse(std::string_view text)
{
    Parser parser;
    parser.text = text;
    const int root = parser.parseValue(0);
    parser.skip();
    Document result;
    if (root == 0 && parser.pos == text.size()) {
        result = parser.document;
    }
    return result;
}

} // namespace json
