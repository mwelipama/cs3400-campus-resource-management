#ifndef LINKEDLIST_H
#define LINKEDLIST_H

// Singly linked list with a tail pointer: insert at either end is O(1); position insert/remove and search are O(n).
template <typename T>
class LinkedList {
private:
    struct Node {
        T     value;
        Node* next;
        explicit Node(const T& newValue) : value(newValue), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    int   count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    ~LinkedList() { clear(); }

    void insertAtBeginning(const T& value) {
        Node* newNode = new Node(value);
        newNode->next = head;
        head = newNode;
        if (tail == nullptr) tail = newNode;
        ++count;
    }

    void insertAtEnd(const T& value) {
        Node* newNode = new Node(value);
        if (tail != nullptr) tail->next = newNode;
        else head = newNode;
        tail = newNode;
        ++count;
    }

    // position 0 = front, position size() = end. Returns false if out of range.
    bool insertAtPosition(int position, const T& value) {
        if (position < 0 || position > count) return false;
        if (position == 0) { insertAtBeginning(value); return true; }
        if (position == count) { insertAtEnd(value); return true; }
        Node* previous = head;
        for (int step = 0; step < position - 1; ++step) previous = previous->next;
        Node* newNode = new Node(value);
        newNode->next = previous->next;
        previous->next = newNode;
        ++count;
        return true;
    }

    bool removeAtPosition(int position, T& removedValue) {
        if (position < 0 || position >= count) return false;
        Node* nodeToDelete = nullptr;
        if (position == 0) {
            nodeToDelete = head;
            head = head->next;
            if (head == nullptr) tail = nullptr;
        } else {
            Node* previous = head;
            for (int step = 0; step < position - 1; ++step) previous = previous->next;
            nodeToDelete = previous->next;
            previous->next = nodeToDelete->next;
            if (nodeToDelete == tail) tail = previous;
        }
        removedValue = nodeToDelete->value;
        delete nodeToDelete;
        --count;
        return true;
    }

    template <typename Predicate>
    int findIndex(Predicate matches) const {
        int index = 0;
        for (Node* current = head; current != nullptr; current = current->next, ++index) {
            if (matches(current->value)) return index;
        }
        return -1;
    }

    template <typename Visitor>
    void forEach(Visitor visit) const {
        for (Node* current = head; current != nullptr; current = current->next) visit(current->value);
    }

    void clear() {
        while (head != nullptr) {
            Node* next = head->next;
            delete head;
            head = next;
        }
        tail = nullptr;
        count = 0;
    }

    int  size() const { return count; }
    bool isEmpty() const { return count == 0; }
};
#endif
