#include "Tree.h"
#include "Queue.h"

void ResourceBST::updateHeight(TreeNode* node) {
    int leftHeight = heightOfNode(node->left);
    int rightHeight = heightOfNode(node->right);
    node->nodeHeight = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
}

int ResourceBST::balanceFactor(const TreeNode* node) {
    return node == nullptr ? 0 : heightOfNode(node->left) - heightOfNode(node->right);
}

ResourceBST::TreeNode* ResourceBST::rotateRight(TreeNode* node) {
    TreeNode* pivot = node->left;
    node->left = pivot->right;
    pivot->right = node;
    updateHeight(node);
    updateHeight(pivot);
    return pivot;
}

ResourceBST::TreeNode* ResourceBST::rotateLeft(TreeNode* node) {
    TreeNode* pivot = node->right;
    node->right = pivot->left;
    pivot->left = node;
    updateHeight(node);
    updateHeight(pivot);
    return pivot;
}

ResourceBST::TreeNode* ResourceBST::rebalance(TreeNode* node) {
    updateHeight(node);
    int balance = balanceFactor(node);
    if (balance > 1) {                                   // left heavy
        if (balanceFactor(node->left) < 0) node->left = rotateLeft(node->left);   // left-right case
        return rotateRight(node);                        // left-left case
    }
    if (balance < -1) {                                  // right heavy
        if (balanceFactor(node->right) > 0) node->right = rotateRight(node->right);  // right-left case
        return rotateLeft(node);                         // right-right case
    }
    return node;
}

ResourceBST::TreeNode* ResourceBST::insertNode(TreeNode* node, LibraryResource* resource, bool& wasInserted) {
    if (node == nullptr) {
        wasInserted = true;
        return new TreeNode(resource);
    }
    int comparison = resource->getId().compare(node->resource->getId());
    if (comparison < 0) node->left = insertNode(node->left, resource, wasInserted);
    else if (comparison > 0) node->right = insertNode(node->right, resource, wasInserted);
    else {
        wasInserted = false;
        return node;
    }
    return rebalance(node);
}

ResourceBST::TreeNode* ResourceBST::removeNode(TreeNode* node, const string& id, bool& wasRemoved) {
    if (node == nullptr) return nullptr;
    int comparison = id.compare(node->resource->getId());
    if (comparison < 0) {
        node->left = removeNode(node->left, id, wasRemoved);
    } else if (comparison > 0) {
        node->right = removeNode(node->right, id, wasRemoved);
    } else {
        wasRemoved = true;
        if (node->left == nullptr) {            // 0 or 1 child (right)
            TreeNode* replacement = node->right;
            delete node;
            return replacement;
        }
        if (node->right == nullptr) {           // 1 child (left)
            TreeNode* replacement = node->left;
            delete node;
            return replacement;
        }
        // 2 children: replace with the inorder successor (smallest node in right subtree)
        TreeNode* successor = node->right;
        while (successor->left != nullptr) successor = successor->left;
        node->resource = successor->resource;
        bool ignored = false;
        node->right = removeNode(node->right, successor->resource->getId(), ignored);
    }
    return rebalance(node);
}

void ResourceBST::inorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const {
    if (node == nullptr) return;
    inorderVisit(node->left, output);
    output.pushBack(node->resource);
    inorderVisit(node->right, output);
}

void ResourceBST::preorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const {
    if (node == nullptr) return;
    output.pushBack(node->resource);
    preorderVisit(node->left, output);
    preorderVisit(node->right, output);
}

void ResourceBST::postorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const {
    if (node == nullptr) return;
    postorderVisit(node->left, output);
    postorderVisit(node->right, output);
    output.pushBack(node->resource);
}

void ResourceBST::destroy(TreeNode* node) {
    if (node == nullptr) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

bool ResourceBST::insert(LibraryResource* resource) {
    bool wasInserted = false;
    root = insertNode(root, resource, wasInserted);
    if (wasInserted) ++nodeCount;
    return wasInserted;
}

bool ResourceBST::remove(const string& id) {
    bool wasRemoved = false;
    root = removeNode(root, id, wasRemoved);
    if (wasRemoved) --nodeCount;
    return wasRemoved;
}

LibraryResource* ResourceBST::search(const string& id, int* comparisons) const {
    int steps = 0;
    TreeNode* current = root;
    while (current != nullptr) {
        ++steps;
        int comparison = id.compare(current->resource->getId());
        if (comparison == 0) {
            if (comparisons) *comparisons = steps;
            return current->resource;
        }
        current = (comparison < 0) ? current->left : current->right;
    }
    if (comparisons) *comparisons = steps;
    return nullptr;
}

void ResourceBST::levelOrder(DynamicArray<LibraryResource*>& output) const {
    if (root == nullptr) return;
    Queue<TreeNode*> pendingNodes;
    pendingNodes.enqueue(root);
    TreeNode* current = nullptr;
    while (pendingNodes.dequeue(current)) {
        output.pushBack(current->resource);
        if (current->left != nullptr) pendingNodes.enqueue(current->left);
        if (current->right != nullptr) pendingNodes.enqueue(current->right);
    }
}
