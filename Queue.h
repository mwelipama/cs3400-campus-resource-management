#ifndef QUEUE_H
#define QUEUE_H

template <typename T>
class Queue {
private:
    struct Node {
        T     value;
        Node* next;
        explicit Node(const T& newValue) : value(newValue), next(nullptr) {}
    };

    Node* frontNode;
    Node* rearNode;
    int   count;

public:
    Queue() : frontNode(nullptr), rearNode(nullptr), count(0) {}
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    ~Queue() { clear(); }

    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        if (rearNode != nullptr) rearNode->next = newNode;
        else frontNode = newNode;
        rearNode = newNode;
        ++count;
    }

    bool dequeue(T& removedValue) {
        if (frontNode == nullptr) return false;
        Node* nodeToDelete = frontNode;
        removedValue = nodeToDelete->value;
        frontNode = frontNode->next;
        if (frontNode == nullptr) rearNode = nullptr;
        delete nodeToDelete;
        --count;
        return true;
    }

    bool peekFront(T& frontValue) const {
        if (frontNode == nullptr) return false;
        frontValue = frontNode->value;
        return true;
    }

    template <typename Predicate>
    bool removeFirstMatching(Predicate matches, T& removedValue) {
        Node* previous = nullptr;
        for (Node* current = frontNode; current != nullptr; previous = current, current = current->next) {
            if (matches(current->value)) {
                if (previous == nullptr) frontNode = current->next;
                else previous->next = current->next;
                if (current == rearNode) rearNode = previous;
                removedValue = current->value;
                delete current;
                --count;
                return true;
            }
        }
        return false;
    }

    template <typename Visitor>
    void forEach(Visitor visit) const {
        for (Node* current = frontNode; current != nullptr; current = current->next) visit(current->value);
    }

    void clear() {
        T discarded;
        while (dequeue(discarded)) {}
    }

    bool isEmpty() const { return count == 0; }
    int  size() const { return count; }
};
#endif
