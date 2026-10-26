#ifndef GUICONTROLLER_H
#define GUICONTROLLER_H

// GuiController: the non-UI bridge between the Qt interface and the project's
// ORIGINAL data structures and algorithms. It owns one instance of every
// structure (exactly as the console LibrarySystem does), loads and saves the
// same text files in the same format, and exposes operations that RETURN data
// or text instead of printing to the console. No algorithm is reimplemented:
// the linked list, AVL tree, hash table, queue, heap, stack, graph, sorting
// and searching code are the real project files, linked unchanged.

#include <string>
#include <vector>

#include "LinkedList.h"
#include "Queue.h"
#include "Stack.h"
#include "PriorityQueue.h"
#include "Resource.h"
#include "Request.h"
#include "Tree.h"
#include "HashTable.h"
#include "Graph.h"

// Plain rows handed to the UI (kept free of Qt so the core stays portable).
struct ResourceRow {
    std::string id, name, type, location, status, extraLabel, extraValue;
    int capacity = 0;
};

struct RequestRow {
    int id = 0;
    std::string user, resourceId, time, status, priority;
    int priorityNum = 0;
};

struct CompareRow {
    std::string method;
    bool found = false;
    int comparisons = 0;
    std::string complexity;
};

struct BenchRow {
    int n = 0;
    long long insertion = 0, merge = 0, quick = 0, linear = 0, binary = 0, bst = 0, hash = 0;
};

// Undo model, mirroring the console's UndoRecord (snapshot owned until consumed).
enum class GuiUndoType { ADDED, DELETED, UPDATED };
struct GuiUndoRecord {
    GuiUndoType      type = GuiUndoType::ADDED;
    LibraryResource* snapshot = nullptr;
    int              position = 0;
};

class GuiController {
public:
    explicit GuiController(const std::string& dataDirectory);
    GuiController(const GuiController&) = delete;
    GuiController& operator=(const GuiController&) = delete;
    ~GuiController();

    // --- persistence (same file format as the console app) ---
    bool load(std::string& report);     // true if the resources file opened
    bool save(std::string& report);
    const std::string& dataDirectory() const { return dataDir; }

    // --- dashboard counts ---
    int resourceCount() const;
    int countByStatus(AvailabilityStatus status) const;
    int countByTypeCode(const std::string& typeCode) const;
    int normalQueueSize() const;
    int priorityHeapSize() const;
    int locationCount() const;
    int pathwayCount() const;
    int historySize() const;

    // --- resources ---
    std::vector<ResourceRow> listResources() const;
    bool addResource(const std::string& typeCode, const std::string& id, const std::string& name,
                     const std::string& location, int capacity, AvailabilityStatus status,
                     const std::string& extraValue, int position, std::string& message);
    bool removeResource(const std::string& id, std::string& message);
    // Empty text keeps the current value; capacity < 0 keeps; statusChoice 0 keeps (1/2/3 set).
    bool updateResource(const std::string& id, const std::string& name, const std::string& location,
                        int capacity, int statusChoice, const std::string& extraValue, std::string& message);
    bool undo(std::string& message);
    int  undoDepth() const { return undoStack.size(); }

    // --- search & sort ---
    std::vector<ResourceRow> searchByName(const std::string& keyword, int& examined) const;
    std::vector<CompareRow>  compareSearch(const std::string& id, bool& found) const;
    // field: 0 id, 1 name, 2 capacity, 3 type; algo: 1 insertion, 2 merge, 3 quick.
    std::vector<ResourceRow> sortedResources(int field, int algo, long long& micros) const;

    // --- requests ---
    std::vector<RequestRow> normalQueueRows() const;
    std::vector<RequestRow> priorityRows() const;   // in served (priority) order
    std::vector<RequestRow> historyRows() const;
    int  submitNormal(const std::string& user, const std::string& resourceId, std::string& message);
    int  submitPriority(const std::string& user, const std::string& resourceId, PriorityLevel level,
                        std::string& message);
    bool processNextNormal(std::string& outcome);
    int  processAllNormal(std::vector<std::string>& outcomes);
    bool processNextPriority(std::string& outcome);
    int  processAllPriority(std::vector<std::string>& outcomes);
    bool cancelNormal(int requestId, std::string& message);
    bool returnResource(const std::string& id, std::string& message);

    // --- AVL tree ---
    std::vector<std::string> traversal(int which) const;   // 0 in,1 pre,2 post,3 level
    int  treeHeight() const { return resourceTree.height(); }
    int  treeSize() const { return resourceTree.size(); }
    bool treeSearch(const std::string& id, int& comparisons, ResourceRow& found) const;

    // --- hash table ---
    std::string hashStatistics() const;   // the real printStatistics() output, captured

    // --- graph ---
    std::vector<std::string> locationList() const;
    std::string graphDescription() const; // locations + pathways text (writeTo)
    std::vector<std::string> bfs(int start) const;
    std::vector<std::string> dfs(int start) const;
    bool reachable(int fromIndex, int toIndex) const;
    int  shortestPath(int fromIndex, int toIndex, std::vector<std::string>& path) const; // metres, -1 if none
    std::string connectedComponents() const;

    // --- analysis ---
    std::vector<BenchRow> runBenchmark() const;

private:
    // The linked list OWNS every resource; the tree and hash table hold extra pointers.
    LinkedList<LibraryResource*>   resourceList;
    ResourceBST                    resourceTree;
    ResourceHashTable              resourceHashTable;
    Queue<ResourceRequest>         normalRequestQueue;
    PriorityQueue<ResourceRequest> priorityRequestHeap;
    LinkedList<ResourceRequest>    processedRequestHistory;
    Stack<GuiUndoRecord>           undoStack;
    CampusGraph                    campusGraph;
    int                            nextRequestId;

    std::string dataDir, resourcesPath, requestsPath, locationsPath;

    bool insertResource(LibraryResource* resource, int position);
    LibraryResource* detachResource(const std::string& id, int& oldPosition);
    void pushUndo(GuiUndoType type, LibraryResource* snapshot, int position);
    ResourceRow rowOf(LibraryResource* r) const;
    std::string evaluate(ResourceRequest& request);   // mirrors evaluateRequest, returns the reason line
    std::vector<ResourceRow> collectRows() const;
};

#endif
