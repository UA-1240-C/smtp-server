#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class JSON {
public:
    enum Type { OBJECT, STRING, NUMBER, BOOL, NIL }; // Enum to define the type of JSON value

    JSON() : type(NIL) {} // Default constructor for NIL type
    JSON(Type t) : type(t) {} // Constructor for different types
    JSON(const std::string &value) : type(STRING), str_value(value) {} // Constructor for STRING type
    JSON(double value) : type(NUMBER), num_value(value) {} // Constructor for NUMBER type
    JSON(bool value) : type(BOOL), bool_value(value) {} // Constructor for BOOL type

    Type get_Type() const { return type; } // Getter for the type of JSON value

    const std::unordered_map<std::string, JSON>& get_Object_value() const { return object_value; } // Getter for OBJECT value
    const std::string& get_String_value() const { return str_value; } // Getter for STRING value
    double get_Number_value() const { return num_value; } // Getter for NUMBER value
    bool get_Bool_value() const { return bool_value; } // Getter for BOOL value

    static JSON Parse(const std::string &content); // Static method to parse JSON from string

private:
    static JSON ParseObject(std::stringstream &ss); // Method to parse JSON object
    static JSON ParseString(std::stringstream &ss); // Method to parse JSON string
    static JSON ParseNumber(std::stringstream &ss); // Method to parse JSON number
    static JSON ParseBool(std::stringstream &ss); // Method to parse JSON boolean
    static JSON ParseNull(std::stringstream &ss); // Method to parse JSON null
    static void SkipWhitespace(std::stringstream &ss); // Helper method to skip whitespace characters

    Type type; // Type of the JSON value
    std::unordered_map<std::string, JSON> object_value; // Holds OBJECT type values
    std::string str_value; // Holds STRING type values
    double num_value; // Holds NUMBER type values
    bool bool_value; // Holds BOOL type values
};

// Helper method to skip whitespace characters in the stream
void JSON::SkipWhitespace(std::stringstream &ss) {
    while (ss.peek() == ' ' || ss.peek() == '\n' || ss.peek() == '\t' || ss.peek() == '\r') {
        ss.get();
    }
}

// Static method to parse JSON from a string
JSON JSON::Parse(const std::string &content) {
    std::stringstream ss(content);
    SkipWhitespace(ss);
    char c = ss.peek();
    if (c == '{') {
        return ParseObject(ss); // Parse as JSON object
    } else if (c == '"') {
        return ParseString(ss); // Parse as JSON string
    } else if (isdigit(c) || c == '-') {
        return ParseNumber(ss); // Parse as JSON number
    } else if (c == 't' || c == 'f') {
        return ParseBool(ss); // Parse as JSON boolean
    } else if (c == 'n') {
        return ParseNull(ss); // Parse as JSON null
    } else {
        throw std::runtime_error("Invalid JSON input"); // Handle invalid JSON input
    }
}

// Method to parse a JSON object from the stream
JSON JSON::ParseObject(std::stringstream &ss) {
    JSON obj(OBJECT);
    bool firstItem = true;
    ss.get(); // Consume '{'
    SkipWhitespace(ss);
    while (ss.peek() != '}') {
        if (firstItem) {
            firstItem = false;
        } else {
            // Skip to the next comma or end of stream
            while (ss.peek() != ',' && ss.peek() != -1) {
                ss.get();
            }
            if (ss.peek() == -1) {
                break;
            }
            ss.get(); // Consume ','
        }
        SkipWhitespace(ss);
        JSON key = ParseString(ss); // Parse the key
        SkipWhitespace(ss);
        ss.get(); // Consume ':'
        SkipWhitespace(ss);
        obj.object_value[key.get_String_value()] = Parse(ss.str().substr(ss.tellg())); // Parse the value and add to the object
        SkipWhitespace(ss);
        if (ss.peek() == ',') {
            ss.get(); // Consume ','
            SkipWhitespace(ss);
        }
    }
    ss.get(); // Consume '}'
    return obj;
}

// Method to parse a JSON string from the stream
JSON JSON::ParseString(std::stringstream &ss) {
    ss.get(); // Consume '"'
    std::string value;
    while (ss.peek() != '"') {
        value += ss.get();
    }
    ss.get(); // Consume '"'
    return JSON(value);
}

// Method to parse a JSON number from the stream
JSON JSON::ParseNumber(std::stringstream &ss) {
    std::string value;
    while (isdigit(ss.peek()) || ss.peek() == '.' || ss.peek() == '-') {
        value += ss.get();
    }
    double value_double = std::stod(value);
    return JSON(value_double);
}

// Method to parse a JSON boolean from the stream
JSON JSON::ParseBool(std::stringstream &ss) {
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

// Method to parse a JSON null value from the stream
JSON JSON::ParseNull(std::stringstream &ss) {
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
