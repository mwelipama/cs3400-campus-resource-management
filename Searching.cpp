#include "Searching.h"

namespace Searching {

int linearSearchById(const DynamicArray<LibraryResource*>& array, const string& targetId,
                            int& comparisons) {
    comparisons = 0;
    for (int index = 0; index < array.size(); ++index) {
        ++comparisons;
        if (array[index]->getId() == targetId) return index;
    }
    return -1;
}

int binarySearchById(const DynamicArray<LibraryResource*>& sortedArray, const string& targetId,
                            int& comparisons) {
    comparisons = 0;
    int low = 0, high = sortedArray.size() - 1;
    while (low <= high) {
        int middle = low + (high - low) / 2;
        ++comparisons;
        int comparison = targetId.compare(sortedArray[middle]->getId());
        if (comparison == 0) return middle;
        if (comparison < 0) high = middle - 1;
        else low = middle + 1;
    }
    return -1;
}

}
