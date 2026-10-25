#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "DynamicArray.h"

using namespace std;

namespace Utils {

// Thrown when standard input is closed (Ctrl+D or end of piped input), so the program can save and exit.
struct InputClosedException {};

template <typename T>
void swapValues(T& first, T& second) {
    T temporary = first;
    first = second;
    second = temporary;
}

string trim(const string& text);

string toLower(string text);

string toUpper(string text);

// '|' is the field separator in the data files, so it is not allowed inside text.
string sanitizeField(string text);

string truncate(const string& text, size_t maxLength);

void printLine(char symbol = '-', int width = 100);

string readLine(const string& prompt);

string readNonEmpty(const string& prompt);

bool tryParseInt(const string& text, int minValue, int maxValue, int& result);

long long parseLongLong(const string& text);

int readInt(const string& prompt, int minValue, int maxValue);

DynamicArray<string> splitFields(const string& line, char delimiter = '|');

void printHeader(const string& title);

}
#endif
