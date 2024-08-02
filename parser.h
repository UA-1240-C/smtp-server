#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class JSON {
public:
    enum Type { OBJECT, STRING, NUMBER, BOOL, NIL };

    JSON() : type(NIL) {}
    JSON(Type t) : type(t) {}
    JSON(const std::string &value) : type(STRING), str_value(value) {}
    JSON(double value) : type(NUMBER), num_value(value) {}
    JSON(bool value) : type(BOOL), bool_value(value) {}

    Type type;

    std::unordered_map<std::string, JSON> object_value;
    std::string str_value;
    double num_value;
    bool bool_value;

    static JSON parse(const std::string &content);
private:
    static JSON parseObject(std::stringstream &ss);
    static JSON parseString(std::stringstream &ss);
    static JSON parseNumber(std::stringstream &ss);
    static JSON parseBool(std::stringstream &ss);
    static JSON parseNull(std::stringstream &ss);
    static void skipWhitespace(std::stringstream &ss);
};

void JSON::skipWhitespace(std::stringstream &ss) {
    while (ss.peek() == ' ' || ss.peek() == '\n' || ss.peek() == '\t' || ss.peek() == '\r') {
        ss.get();
    }
}

JSON JSON::parse(const std::string &content) {
    std::stringstream ss(content);
    skipWhitespace(ss);
    char c = ss.peek();
    if (c == '{') {
        return parseObject(ss);
    } else if (c == '"') {
        return parseString(ss);
    } else if (isdigit(c) || c == '-') {
        return parseNumber(ss);
    } else if (c == 't' || c == 'f') {
        return parseBool(ss);
    } else if (c == 'n') {
        return parseNull(ss);
    } else {

        throw std::runtime_error("Invalid JSON input");
    }
}

JSON JSON::parseObject(std::stringstream &ss) {
    JSON obj(OBJECT);
    bool firtsItem = true;
    ss.get(); // consume '{'
    skipWhitespace(ss);
    while (ss.peek() != '}') {
        if(firtsItem){
            firtsItem = false;
        } else {
            while(ss.peek() != ',' && ss.peek() != -1){
                ss.get();
            }
            if(ss.peek() == -1){
                break;
            }
            ss.get(); // consume ','
        }
        skipWhitespace(ss);
        JSON key = parseString(ss);
        skipWhitespace(ss);
        ss.get(); // consume ':'
        skipWhitespace(ss);
        obj.object_value[key.str_value] = parse(ss.str().substr(ss.tellg()));
        skipWhitespace(ss);
        if (ss.peek() == ',') {
            ss.get(); // consume ','
            skipWhitespace(ss);
        }
    }
    ss.get(); // consume '}'
    return obj;
}

JSON JSON::parseString(std::stringstream &ss) {
    ss.get(); // consume '"'
    std::string value;
    while (ss.peek() != '"') {
        value += ss.get();
    }
    ss.get(); // consume '"'
    return JSON(value);
}

JSON JSON::parseNumber(std::stringstream &ss) {
    std::string value;
    while (isdigit(ss.peek()) || ss.peek() == '.' || ss.peek() == '-') {
        value += ss.get();
    }
    double value_double = std::stod(value);
    return JSON(value_double);
}

JSON JSON::parseBool(std::stringstream &ss) {
    std::string value;
    while (isalpha(ss.peek())) {
        value += ss.get();
    }
    if (value == "true") {
        return JSON(true);
    } else if (value == "false") {
        return JSON(false);
    } else {
        throw std::runtime_error("Invalid JSON input");
    }
}

JSON JSON::parseNull(std::stringstream &ss) {
    std::string value;
    while (isalpha(ss.peek())) {
        value += ss.get();
    }
    if (value == "null") {
        return JSON();
    } else {
        throw std::runtime_error("Invalid JSON input");
    }
}