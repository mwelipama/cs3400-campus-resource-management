#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include "DynamicArray.h"
#include "Utils.h"

// Binary min-heap. T must define operator<, where a < b means a is more urgent than b.
// insert and extractTop are O(log n), peekTop is O(1).
template <typename T>
class PriorityQueue {
private:
    DynamicArray<T> heapItems;

    void siftUp(int index) {
        while (index > 0) {
            int parentIndex = (index - 1) / 2;
            if (heapItems[index] < heapItems[parentIndex]) {
                Utils::swapValues(heapItems[index], heapItems[parentIndex]);
                index = parentIndex;
            } else {
                break;
            }
        }
    }

    void siftDown(int index) {
        int itemCount = heapItems.size();
        while (true) {
            int leftChild = 2 * index + 1;
            int rightChild = leftChild + 1;
            int mostUrgent = index;
            if (leftChild < itemCount && heapItems[leftChild] < heapItems[mostUrgent]) mostUrgent = leftChild;
            if (rightChild < itemCount && heapItems[rightChild] < heapItems[mostUrgent]) mostUrgent = rightChild;
            if (mostUrgent == index) break;
            Utils::swapValues(heapItems[index], heapItems[mostUrgent]);
            index = mostUrgent;
        }
    }

public:
    void insert(const T& item) {
        heapItems.pushBack(item);
        siftUp(heapItems.size() - 1);
    }

    bool extractTop(T& topItem) {
        if (heapItems.isEmpty()) return false;
        topItem = heapItems[0];
        heapItems[0] = heapItems[heapItems.size() - 1];
        heapItems.removeLast();
        if (!heapItems.isEmpty()) siftDown(0);
        return true;
    }

    bool peekTop(T& topItem) const {
        if (heapItems.isEmpty()) return false;
        topItem = heapItems[0];
        return true;
    }

    // Non-destructive: copy the heap and extract everything (heap-sort order).
    DynamicArray<T> toSortedArray() const {
        PriorityQueue copy(*this);
        DynamicArray<T> sortedItems;
        T item;
        while (copy.extractTop(item)) sortedItems.pushBack(item);
        return sortedItems;
    }

    void copyItemsTo(DynamicArray<T>& destination) const {
        for (int index = 0; index < heapItems.size(); ++index) destination.pushBack(heapItems[index]);
    }

    bool isEmpty() const { return heapItems.isEmpty(); }
    int  size() const { return heapItems.size(); }
};
#endif
