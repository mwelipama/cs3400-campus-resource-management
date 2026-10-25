#ifndef SEARCHING_H
#define SEARCHING_H

#include <string>
#include "DynamicArray.h"
#include "Resource.h"

using namespace std;

// linearSearch is O(n); binarySearch is O(log n) and requires a sorted array.
namespace Searching {

int linearSearchById(const DynamicArray<LibraryResource*>& array, const string& targetId,
                            int& comparisons);

// The array MUST already be sorted by ID (ascending).
int binarySearchById(const DynamicArray<LibraryResource*>& sortedArray, const string& targetId,
                            int& comparisons);

}
#endif
