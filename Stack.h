#ifndef STACK_H
#define STACK_H

template <typename T>
class Stack {
private:
    struct Node {
        T     value;
        Node* below;
        Node(const T& newValue, Node* nodeBelow) : value(newValue), below(nodeBelow) {}
    };

    Node* topNode;
    int   count;

public:
    Stack() : topNode(nullptr), count(0) {}
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;
    ~Stack() {
        while (topNode != nullptr) {
            Node* below = topNode->below;
            delete topNode;
            topNode = below;
        }
    }

    void push(const T& value) {
        topNode = new Node(value, topNode);
        ++count;
    }

    bool pop(T& poppedValue) {
        if (topNode == nullptr) return false;
        Node* nodeToDelete = topNode;
        poppedValue = nodeToDelete->value;
        topNode = nodeToDelete->below;
        delete nodeToDelete;
        --count;
        return true;
    }

    bool peek(T& topValue) const {
        if (topNode == nullptr) return false;
        topValue = topNode->value;
        return true;
    }

    template <typename Visitor>
    void forEach(Visitor visit) const {
        for (Node* current = topNode; current != nullptr; current = current->below) visit(current->value);
    }

    bool isEmpty() const { return count == 0; }
    int  size() const { return count; }
};
#endif
