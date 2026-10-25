#include "Utils.h"
#include <iostream>

namespace Utils {

string trim(const string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

string toLower(string text) {
    for (char& character : text) {
        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character - 'A' + 'a');
    }
    return text;
}

string toUpper(string text) {
    for (char& character : text) {
        if (character >= 'a' && character <= 'z') character = static_cast<char>(character - 'a' + 'A');
    }
    return text;
}

string sanitizeField(string text) {
    for (char& character : text) {
        if (character == '|') character = '/';
    }
    return trim(text);
}

string truncate(const string& text, size_t maxLength) {
    if (text.size() <= maxLength) return text;
    return text.substr(0, maxLength - 2) + "..";
}

void printLine(char symbol, int width) {
    cout << string(width, symbol) << "\n";
}

string readLine(const string& prompt) {
    cout << prompt << flush;
    string line;
    if (!getline(cin, line)) throw InputClosedException();
    return trim(line);
}

string readNonEmpty(const string& prompt) {
    while (true) {
        string value = sanitizeField(readLine(prompt));
        if (!value.empty()) return value;
        cout << "  Input cannot be empty.\n";
    }
}

bool tryParseInt(const string& text, int minValue, int maxValue, int& result) {
    try {
        size_t charactersUsed = 0;
        int value = stoi(text, &charactersUsed);
        if (charactersUsed == text.size() && value >= minValue && value <= maxValue) {
            result = value;
            return true;
        }
    } catch (...) {
    }
    return false;
}

long long parseLongLong(const string& text) {
    long long value = 0;
    for (char character : text) {
        if (character < '0' || character > '9') return 0;
        value = value * 10 + (character - '0');
    }
    return value;
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        int value = 0;
        if (tryParseInt(readLine(prompt), minValue, maxValue, value)) return value;
        cout << "  Invalid input. Enter a whole number between " << minValue << " and " << maxValue << ".\n";
    }
}

DynamicArray<string> splitFields(const string& line, char delimiter) {
    DynamicArray<string> fields;
    string current;
    for (char character : line) {
        if (character == delimiter) {
            fields.pushBack(current);
            current.clear();
        } else {
            current += character;
        }
    }
    fields.pushBack(current);
    return fields;
}

void printHeader(const string& title) {
    cout << "\n";
    printLine('=', 60);
    cout << "  " << title << "\n";
    printLine('=', 60);
}

}
