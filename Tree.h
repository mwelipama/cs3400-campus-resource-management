#ifndef TREE_H
#define TREE_H

#include <string>
#include "DynamicArray.h"
#include "Resource.h"

using namespace std;

// AVL tree keyed by resource ID: rotations keep the height O(log n) even when IDs arrive in sorted order.
// The tree stores pointers only and does not own the resources.
class ResourceBST {
private:
    struct TreeNode {
        LibraryResource* resource;
        TreeNode*        left;
        TreeNode*        right;
        int              nodeHeight;   // height of the subtree rooted here (leaf = 1)
        explicit TreeNode(LibraryResource* newResource)
    : resource(newResource), left(nullptr), right(nullptr), nodeHeight(1) {}
    };

    TreeNode* root;
    int       nodeCount;

    static int heightOfNode(const TreeNode* node) { return node == nullptr ? 0 : node->nodeHeight; }

    static void updateHeight(TreeNode* node);

    static int balanceFactor(const TreeNode* node);

    // Right rotation: the left child (pivot) becomes the new root of this subtree.
    //    before: node(pivot(A, B), C)      after: pivot(A, node(B, C))
    static TreeNode* rotateRight(TreeNode* node);

    static TreeNode* rotateLeft(TreeNode* node);

    static TreeNode* rebalance(TreeNode* node);

    TreeNode* insertNode(TreeNode* node, LibraryResource* resource, bool& wasInserted);

    TreeNode* removeNode(TreeNode* node, const string& id, bool& wasRemoved);

    void inorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const;

    void preorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const;

    void postorderVisit(TreeNode* node, DynamicArray<LibraryResource*>& output) const;

    void destroy(TreeNode* node);

public:
    ResourceBST() : root(nullptr), nodeCount(0) {}
    ResourceBST(const ResourceBST&) = delete;
    ResourceBST& operator=(const ResourceBST&) = delete;
    ~ResourceBST() { destroy(root); }

    bool insert(LibraryResource* resource);

    bool remove(const string& id);

    LibraryResource* search(const string& id, int* comparisons = nullptr) const;

    void inorder(DynamicArray<LibraryResource*>& output) const { inorderVisit(root, output); }
    void preorder(DynamicArray<LibraryResource*>& output) const { preorderVisit(root, output); }
    void postorder(DynamicArray<LibraryResource*>& output) const { postorderVisit(root, output); }

    void levelOrder(DynamicArray<LibraryResource*>& output) const;

    int  height() const { return heightOfNode(root); }
    int  size() const { return nodeCount; }
    bool isEmpty() const { return root == nullptr; }
};
#endif
