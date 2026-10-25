#include "HashTable.h"
#include <iostream>
#include <iomanip>

int ResourceHashTable::hashFunction(const string& key) const {
    unsigned long hashValue = 5381;
    for (char character : key) hashValue = hashValue * 33 + static_cast<unsigned char>(character);
    return static_cast<int>(hashValue % static_cast<unsigned long>(bucketCount));
}

ResourceHashTable::ResourceHashTable(int tableSize)
    : buckets(nullptr), bucketCount(tableSize > 0 ? tableSize : 1), itemCount(0), collisionCount(0) {
    buckets = new ChainNode*[bucketCount];
    for (int index = 0; index < bucketCount; ++index) buckets[index] = nullptr;
}

ResourceHashTable::~ResourceHashTable() {
    for (int index = 0; index < bucketCount; ++index) {
        ChainNode* current = buckets[index];
        while (current != nullptr) {
            ChainNode* next = current->next;
            delete current;
            current = next;
        }
    }
    delete[] buckets;
}

bool ResourceHashTable::insert(LibraryResource* resource) {
    const string& key = resource->getId();
    int bucketIndex = hashFunction(key);
    for (ChainNode* current = buckets[bucketIndex]; current != nullptr; current = current->next) {
        if (current->key == key) return false;
    }
    ChainNode* newNode = new ChainNode(key, resource);
    if (buckets[bucketIndex] != nullptr) ++collisionCount;
    newNode->next = buckets[bucketIndex];
    buckets[bucketIndex] = newNode;
    ++itemCount;
    return true;
}

LibraryResource* ResourceHashTable::find(const string& key, int* comparisons) const {
    int steps = 0;
    for (ChainNode* current = buckets[hashFunction(key)]; current != nullptr; current = current->next) {
        ++steps;
        if (current->key == key) {
            if (comparisons) *comparisons = steps;
            return current->resource;
        }
    }
    if (comparisons) *comparisons = steps;
    return nullptr;
}

bool ResourceHashTable::remove(const string& key) {
    int bucketIndex = hashFunction(key);
    ChainNode* previous = nullptr;
    for (ChainNode* current = buckets[bucketIndex]; current != nullptr; previous = current, current = current->next) {
        if (current->key == key) {
            if (previous == nullptr) buckets[bucketIndex] = current->next;
            else previous->next = current->next;
            delete current;
            --itemCount;
            return true;
        }
    }
    return false;
}

void ResourceHashTable::printStatistics() const {
    int emptyBuckets = 0, longestChain = 0;
    for (int index = 0; index < bucketCount; ++index) {
        int chainLength = 0;
        for (ChainNode* current = buckets[index]; current != nullptr; current = current->next) ++chainLength;
        if (chainLength == 0) ++emptyBuckets;
        if (chainLength > longestChain) longestChain = chainLength;
    }
    cout << "  Table size (buckets)   : " << bucketCount << "\n"
              << "  Stored resources       : " << itemCount << "\n"
              << "  Load factor            : " << fixed << setprecision(2)
              << static_cast<double>(itemCount) / bucketCount << "\n"
              << "  Empty buckets          : " << emptyBuckets << "\n"
              << "  Collisions so far      : " << collisionCount << "\n"
              << "  Longest chain          : " << longestChain << "\n"
              << "  Collision technique    : separate chaining\n"
              << "  Hash function          : djb2 modulo table size\n";
}

void ResourceHashTable::printBuckets() const {
    for (int index = 0; index < bucketCount; ++index) {
        if (buckets[index] == nullptr) continue;
        cout << "  [" << right << setw(2) << index << left << "] ";
        for (ChainNode* current = buckets[index]; current != nullptr; current = current->next) {
            cout << current->key << (current->next ? " -> " : "");
        }
        cout << "\n";
    }
}
