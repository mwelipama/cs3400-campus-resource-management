#ifndef LIBRARYSYSTEM_H
#define LIBRARYSYSTEM_H

#include <string>
#include "DynamicArray.h"
#include "LinkedList.h"
#include "Queue.h"
#include "Stack.h"
#include "PriorityQueue.h"
#include "Resource.h"
#include "Request.h"
#include "Sorting.h"
#include "Searching.h"
#include "Tree.h"
#include "HashTable.h"
#include "Graph.h"

using namespace std;

enum class UndoActionType { ADDED_RESOURCE, DELETED_RESOURCE, UPDATED_RESOURCE };

struct UndoRecord {
    UndoActionType   actionType = UndoActionType::ADDED_RESOURCE;
    LibraryResource* resourceSnapshot = nullptr;   // owned by the undo stack until the record is consumed
    int              originalPosition = 0;         // where a deleted resource sat in the linked list
};

class LibrarySystem {
private:
    // The linked list is the OWNER of every resource object.
    // The BST and the hash table keep extra pointers for fast searching.
    LinkedList<LibraryResource*>  resourceList;
    ResourceBST                   resourceTree;
    ResourceHashTable             resourceHashTable;
    Queue<ResourceRequest>        normalRequestQueue;
    PriorityQueue<ResourceRequest> priorityRequestHeap;
    LinkedList<ResourceRequest>   processedRequestHistory;
    Stack<UndoRecord>             undoStack;
    CampusGraph                   campusGraph;
    int                           nextRequestId;

    const string resourceFilePath = "data/resources.txt";
    const string requestFilePath = "data/requests.txt";
    const string locationFilePath = "data/locations.txt";

    DynamicArray<LibraryResource*> collectResources() const;

    // Inserts a resource into all three structures. position: index in the linked list.
    bool insertResource(LibraryResource* resource, int position);

    // Removes a resource from all three structures but does NOT delete the object.
    LibraryResource* detachResource(const string& id, int& oldPosition);

    void pushUndo(UndoActionType type, LibraryResource* snapshot, int position);

    static string describeUndoRecord(const UndoRecord& record);

    AvailabilityStatus promptForStatus() const;

    LibraryResource* promptForNewResource() const;

    void printResourceTable(const DynamicArray<LibraryResource*>& resources) const;

    void printPathOfLocations(const DynamicArray<int>& order, const string& separator) const;

    int chooseLocation(const string& prompt) const;

    static long long currentMicroseconds();

    enum class InsertMode { BEGINNING, END, POSITION };

    void addResource(InsertMode mode);

    void removeResource();

    void updateResource();

    void displayAllResources() const;

    void resourceManagementMenu();

    static bool sortedByIdLess(LibraryResource* first, LibraryResource* second);

    void linearSearchByName() const;

    void binarySearchById() const;

    void compareSearchMethods() const;

    void searchMenu() const;

    template <typename T, typename LessFunction>
    long long runSortingAlgorithm(int algorithmChoice, DynamicArray<T>& data, LessFunction isLess) const {
        long long start = currentMicroseconds();
        if (algorithmChoice == 1) Sorting::insertionSort(data, isLess);
        else if (algorithmChoice == 2) Sorting::mergeSort(data, isLess);
        else Sorting::quickSort(data, isLess);
        return currentMicroseconds() - start;
    }

    static string algorithmName(int algorithmChoice);

    void sortResources() const;

    void sortRequests() const;

    void sortMenu() const;

    void evaluateRequest(ResourceRequest& request);

    bool createRequest(PriorityLevel priority, ResourceRequest& newRequest);

    void submitNormalRequest();

    void viewPendingNormalRequests() const;

    void processNextNormalRequest();

    void processAllNormalRequests();

    void cancelNormalRequest();

    void returnResource();

    void viewRequestHistory() const;

    void requestMenu();

    void submitPriorityRequest();

    void viewPriorityRequests() const;

    void peekHighestPriority() const;

    void processHighestPriorityRequest();

    void processAllPriorityRequests();

    void priorityMenu();

    void undoLastOperation();

    void undoMenu();

    void printIdSequence(const string& title, const DynamicArray<LibraryResource*>& sequence) const;

    void treeMenu() const;

    void hashMenu() const;

    void navigationMenu();

    void printComplexityTable() const;

    void runBenchmark() const;

    void analysisMenu() const;

    void loadSampleData();

    bool loadResourcesFromFile();

    void loadRequestsFromFile();

    void loadLocationsFromFile();

public:
    LibrarySystem() : resourceHashTable(53), nextRequestId(1001) {}
    LibrarySystem(const LibrarySystem&) = delete;
    LibrarySystem& operator=(const LibrarySystem&) = delete;

    ~LibrarySystem();

    void loadAllData();

    void saveAllData() const;

    void run();
};
#endif
