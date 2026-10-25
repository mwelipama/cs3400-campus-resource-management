#include <iostream>
#include "LibrarySystem.h"
#include "Utils.h"

using namespace std;

int main() {
    LibrarySystem librarySystem;
    librarySystem.loadAllData();
    try {
        librarySystem.run();
    } catch (const Utils::InputClosedException&) {
        cout << "\nInput closed - saving and exiting.\n";
    }
    librarySystem.saveAllData();
    cout << "Goodbye!\n";
    return 0;
}
