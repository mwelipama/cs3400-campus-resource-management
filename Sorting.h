#ifndef SORTING_H
#define SORTING_H

#include "DynamicArray.h"
#include "Utils.h"

// isLess(a, b) returns true when a must come before b.
// insertionSort: O(n^2), stable. mergeSort: O(n log n), stable, O(n) extra space. quickSort: O(n log n) average, O(n^2) worst.
namespace Sorting {

template <typename T, typename LessFunction>
void insertionSort(DynamicArray<T>& array, LessFunction isLess) {
    for (int current = 1; current < array.size(); ++current) {
        T keyValue = array[current];
        int position = current - 1;
        while (position >= 0 && isLess(keyValue, array[position])) {
            array[position + 1] = array[position];
            --position;
        }
        array[position + 1] = keyValue;
    }
}

template <typename T, typename LessFunction>
void mergeRanges(DynamicArray<T>& array, DynamicArray<T>& buffer, int left, int middle, int right,
                 LessFunction isLess) {
    int leftIndex = left, rightIndex = middle + 1, bufferIndex = left;
    while (leftIndex <= middle && rightIndex <= right) {
        if (isLess(array[rightIndex], array[leftIndex])) buffer[bufferIndex++] = array[rightIndex++];
        else buffer[bufferIndex++] = array[leftIndex++];   // ties take the left item -> stable
    }
    while (leftIndex <= middle) buffer[bufferIndex++] = array[leftIndex++];
    while (rightIndex <= right) buffer[bufferIndex++] = array[rightIndex++];
    for (int index = left; index <= right; ++index) array[index] = buffer[index];
}

template <typename T, typename LessFunction>
void mergeSortRange(DynamicArray<T>& array, DynamicArray<T>& buffer, int left, int right, LessFunction isLess) {
    if (left >= right) return;
    int middle = left + (right - left) / 2;
    mergeSortRange(array, buffer, left, middle, isLess);
    mergeSortRange(array, buffer, middle + 1, right, isLess);
    mergeRanges(array, buffer, left, middle, right, isLess);
}

template <typename T, typename LessFunction>
void mergeSort(DynamicArray<T>& array, LessFunction isLess) {
    if (array.size() < 2) return;
    DynamicArray<T> buffer(array.size());
    buffer.resize(array.size());
    mergeSortRange(array, buffer, 0, array.size() - 1, isLess);
}

template <typename T, typename LessFunction>
int partitionRange(DynamicArray<T>& array, int low, int high, LessFunction isLess) {
    int middle = low + (high - low) / 2;          // middle pivot avoids the sorted-input worst case
    Utils::swapValues(array[middle], array[high]);
    T pivotValue = array[high];
    int boundary = low - 1;
    for (int scan = low; scan < high; ++scan) {
        if (isLess(array[scan], pivotValue)) {
            ++boundary;
            Utils::swapValues(array[boundary], array[scan]);
        }
    }
    Utils::swapValues(array[boundary + 1], array[high]);
    return boundary + 1;
}

template <typename T, typename LessFunction>
void quickSortRange(DynamicArray<T>& array, int low, int high, LessFunction isLess) {
    if (low >= high) return;
    int pivotIndex = partitionRange(array, low, high, isLess);
    quickSortRange(array, low, pivotIndex - 1, isLess);
    quickSortRange(array, pivotIndex + 1, high, isLess);
}

template <typename T, typename LessFunction>
void quickSort(DynamicArray<T>& array, LessFunction isLess) {
    if (array.size() < 2) return;
    quickSortRange(array, 0, array.size() - 1, isLess);
}

}
#endif
