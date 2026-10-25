#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string>
#include "Resource.h"

using namespace std;

// Hash table using djb2 hashing and separate chaining: average O(1) insert, find and remove.
// It stores pointers only and does not own the resources.
class ResourceHashTable {
private:
    struct ChainNode {
        string      key;
        LibraryResource* resource;
        ChainNode*       next;
        ChainNode(const string& newKey, LibraryResource* newResource)
    : key(newKey), resource(newResource), next(nullptr) {}
    };

    ChainNode** buckets;
    int         bucketCount;
    int         itemCount;
    int         collisionCount;   // insertions that landed in an already used bucket

    int hashFunction(const string& key) const;

public:
    explicit ResourceHashTable(int tableSize = 53);

    ResourceHashTable(const ResourceHashTable&) = delete;
    ResourceHashTable& operator=(const ResourceHashTable&) = delete;

    ~ResourceHashTable();

    bool insert(LibraryResource* resource);

    LibraryResource* find(const string& key, int* comparisons = nullptr) const;

    bool remove(const string& key);

    int size() const { return itemCount; }

    void printStatistics() const;

    void printBuckets() const;
};
#endif
