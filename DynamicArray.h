#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

// Resizable array: capacity doubles when full, so pushBack is amortised O(1).
template <typename T>
class DynamicArray {
public:
    struct IndexOutOfRange {};

private:
    T*  elements;
    int count;
    int capacity;

    void grow() {
        int newCapacity = capacity * 2;
        T* newElements = new T[newCapacity];
        for (int index = 0; index < count; ++index) {
            newElements[index] = elements[index];
        }
        delete[] elements;
        elements = newElements;
        capacity = newCapacity;
    }

public:
    explicit DynamicArray(int initialCapacity = 8)
    : elements(nullptr), count(0), capacity(initialCapacity > 0 ? initialCapacity : 1) {
        elements = new T[capacity];
    }

    DynamicArray(const DynamicArray& other)
    : elements(new T[other.capacity]), count(other.count), capacity(other.capacity) {
        for (int index = 0; index < count; ++index) {
            elements[index] = other.elements[index];
        }
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            T* newElements = new T[other.capacity];
            for (int index = 0; index < other.count; ++index) {
                newElements[index] = other.elements[index];
            }
            delete[] elements;
            elements = newElements;
            count = other.count;
            capacity = other.capacity;
        }
        return *this;
    }

    ~DynamicArray() { delete[] elements; }

    void pushBack(const T& value) {
        if (count == capacity) grow();
        elements[count++] = value;
    }

    void removeLast() {
        if (count > 0) --count;
    }

    // Makes the array "logically" hold newCount slots (used as a work buffer by merge sort).
    void resize(int newCount) {
        while (capacity < newCount) grow();
        count = newCount;
    }

    void clear() { count = 0; }
    int  size()  const { return count; }
    bool isEmpty() const { return count == 0; }

    T& operator[](int index) {
        if (index < 0 || index >= count) throw IndexOutOfRange();
        return elements[index];
    }
    const T& operator[](int index) const {
        if (index < 0 || index >= count) throw IndexOutOfRange();
        return elements[index];
    }
};
#endif
