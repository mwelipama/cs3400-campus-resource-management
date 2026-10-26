#include "GuiController.h"

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>

#include "Sorting.h"
#include "Searching.h"
#include "Utils.h"

using namespace std;

// Runs a callable while std::cout is redirected into a string, so the project's
// real print methods (hash statistics, connected components, graph view) can be
// reused verbatim and their genuine output shown in the GUI.
template <typename Fn>
static string captureCout(Fn action) {
    ostringstream captured;
    streambuf* previous = cout.rdbuf(captured.rdbuf());
    action();
    cout.rdbuf(previous);
    return captured.str();
}

static long long microsNow() {
    return static_cast<long long>(clock()) * 1000000LL / CLOCKS_PER_SEC;
}

GuiController::GuiController(const string& dataDirectory)
: nextRequestId(1001), dataDir(dataDirectory) {
    if (!dataDir.empty() && dataDir.back() != '/' && dataDir.back() != '\\') dataDir += '/';
    resourcesPath = dataDir + "resources.txt";
    requestsPath  = dataDir + "requests.txt";
    locationsPath = dataDir + "locations.txt";
}

GuiController::~GuiController() {
    // The linked list owns the resources; delete each one exactly once.
    resourceList.forEach([](LibraryResource* const& r) { delete r; });
    GuiUndoRecord record;
    while (undoStack.pop(record)) delete record.snapshot;   // detached/snapshot objects
}

// ---------------------------------------------------------------- structure glue
bool GuiController::insertResource(LibraryResource* resource, int position) {
    if (resourceHashTable.find(resource->getId()) != nullptr) return false;
    if (position < 0 || position > resourceList.size()) position = resourceList.size();
    resourceList.insertAtPosition(position, resource);
    resourceTree.insert(resource);
    resourceHashTable.insert(resource);
    return true;
}

LibraryResource* GuiController::detachResource(const string& id, int& oldPosition) {
    oldPosition = resourceList.findIndex(
        [&id](LibraryResource* const& r) { return r->getId() == id; });
    if (oldPosition == -1) return nullptr;
    LibraryResource* detached = nullptr;
    resourceList.removeAtPosition(oldPosition, detached);
    resourceTree.remove(id);
    resourceHashTable.remove(id);
    return detached;
}

void GuiController::pushUndo(GuiUndoType type, LibraryResource* snapshot, int position) {
    GuiUndoRecord record;
    record.type = type;
    record.snapshot = snapshot;
    record.position = position;
    undoStack.push(record);
}

ResourceRow GuiController::rowOf(LibraryResource* r) const {
    ResourceRow row;
    row.id = r->getId();
    row.name = r->getName();
    row.type = r->getTypeName();
    row.location = r->getLocation();
    row.capacity = r->getCapacity();
    row.status = statusToString(r->getStatus());
    row.extraLabel = r->getExtraLabel();
    row.extraValue = r->getExtraValue();
    return row;
}

vector<ResourceRow> GuiController::collectRows() const {
    vector<ResourceRow> rows;
    resourceList.forEach([&](LibraryResource* const& r) { rows.push_back(rowOf(r)); });
    return rows;
}

// ---------------------------------------------------------------- persistence
bool GuiController::load(string& report) {
    ostringstream notes;
    // resources
    bool opened = false;
    {
        ifstream input(resourcesPath);
        opened = input.is_open();
        string line;
        while (opened && getline(input, line)) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            DynamicArray<string> fields = Utils::splitFields(line);
            int capacity = 0;
            if (fields.size() != 7 || !Utils::tryParseInt(fields[4], 0, 100000, capacity)) {
                notes << "skipped a malformed resource line\n"; continue;
            }
            LibraryResource* resource = ResourceFactory::create(fields[0], fields[1], fields[2], fields[3],
                                                                capacity, statusFromString(fields[5]), fields[6]);
            if (resource == nullptr || !insertResource(resource, resourceList.size())) {
                notes << "skipped invalid or duplicate resource line\n"; delete resource;
            }
        }
    }
    // requests
    {
        ifstream input(requestsPath);
        string line;
        while (getline(input, line)) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            DynamicArray<string> fields = Utils::splitFields(line);
            if (fields.size() == 2 && fields[0] == "NEXT") {
                Utils::tryParseInt(fields[1], 1, 100000000, nextRequestId); continue;
            }
            int requestId = 0, priorityNumber = 0;
            if (fields.size() != 7 || !Utils::tryParseInt(fields[1], 1, 100000000, requestId) ||
                !Utils::tryParseInt(fields[6], 1, 4, priorityNumber)) {
                notes << "skipped a malformed request line\n"; continue;
            }
            time_t submitted = static_cast<time_t>(Utils::parseLongLong(fields[4]));
            ResourceRequest request(requestId, fields[2], fields[3], submitted,
                                    requestStatusFromString(fields[5]),
                                    static_cast<PriorityLevel>(priorityNumber));
            if (fields[0] == "N") normalRequestQueue.enqueue(request);
            else priorityRequestHeap.insert(request);
        }
    }
    // locations
    {
        ifstream input(locationsPath);
        string line;
        while (getline(input, line)) {
            line = Utils::trim(line);
            if (line.empty()) continue;
            DynamicArray<string> fields = Utils::splitFields(line);
            int meters = 0;
            if (fields.size() == 2 && fields[0] == "L") {
                campusGraph.addLocation(fields[1]);
            } else if (fields.size() == 4 && fields[0] == "E" &&
                       Utils::tryParseInt(fields[3], 1, 100000, meters)) {
                campusGraph.addConnection(campusGraph.findLocationIndex(fields[1]),
                                          campusGraph.findLocationIndex(fields[2]), meters);
            } else {
                notes << "skipped a malformed location line\n";
            }
        }
    }
    report = notes.str();
    return opened;
}

bool GuiController::save(string& report) {
    ofstream resourceOutput(resourcesPath);
    if (!resourceOutput.is_open()) { report = "Could not write to " + resourcesPath; return false; }
    resourceList.forEach([&](LibraryResource* const& r) { resourceOutput << r->serialize() << "\n"; });

    ofstream requestOutput(requestsPath);
    requestOutput << "NEXT|" << nextRequestId << "\n";
    normalRequestQueue.forEach([&](const ResourceRequest& req) { requestOutput << "N|" << req.serialize() << "\n"; });
    DynamicArray<ResourceRequest> heapContents;
    priorityRequestHeap.copyItemsTo(heapContents);
    for (int i = 0; i < heapContents.size(); ++i) requestOutput << "P|" << heapContents[i].serialize() << "\n";

    ofstream locationOutput(locationsPath);
    campusGraph.writeTo(locationOutput);
    report = "Saved to " + dataDir + " (resources.txt, requests.txt, locations.txt).";
    return true;
}

// ---------------------------------------------------------------- dashboard
int GuiController::resourceCount() const { return resourceList.size(); }
int GuiController::countByStatus(AvailabilityStatus status) const {
    int n = 0;
    resourceList.forEach([&](LibraryResource* const& r) { if (r->getStatus() == status) ++n; });
    return n;
}
int GuiController::countByTypeCode(const string& typeCode) const {
    int n = 0;
    resourceList.forEach([&](LibraryResource* const& r) { if (r->getTypeCode() == typeCode) ++n; });
    return n;
}
int GuiController::normalQueueSize() const { return normalRequestQueue.size(); }
int GuiController::priorityHeapSize() const { return priorityRequestHeap.size(); }
int GuiController::locationCount() const { return campusGraph.locationCount(); }
int GuiController::pathwayCount() const {
    int edges = 0;
    for (int i = 0; i < campusGraph.locationCount(); ++i)
        for (int j = i + 1; j < campusGraph.locationCount(); ++j)
            if (campusGraph.hasConnection(i, j)) ++edges;
    return edges;
}
int GuiController::historySize() const { return processedRequestHistory.size(); }

// ---------------------------------------------------------------- resources
vector<ResourceRow> GuiController::listResources() const { return collectRows(); }

bool GuiController::addResource(const string& typeCode, const string& id, const string& name,
                                const string& location, int capacity, AvailabilityStatus status,
                                const string& extraValue, int position, string& message) {
    LibraryResource* resource = ResourceFactory::create(typeCode, Utils::toUpper(id), name, location,
                                                        capacity, status, extraValue);
    if (resource == nullptr) { message = "Unknown resource type."; return false; }
    if (!insertResource(resource, position)) {
        message = "A resource with ID " + resource->getId() + " already exists.";
        delete resource; return false;
    }
    pushUndo(GuiUndoType::ADDED, resource->clone(), position);
    message = "Resource " + resource->getId() + " added at position " + to_string(position) + ".";
    return true;
}

bool GuiController::removeResource(const string& id, string& message) {
    int oldPosition = 0;
    LibraryResource* detached = detachResource(Utils::toUpper(id), oldPosition);
    if (detached == nullptr) { message = "No resource with ID " + Utils::toUpper(id) + "."; return false; }
    pushUndo(GuiUndoType::DELETED, detached, oldPosition);   // the detached object is the snapshot
    message = "Resource " + Utils::toUpper(id) + " removed (use Undo to restore it).";
    return true;
}

bool GuiController::updateResource(const string& id, const string& name, const string& location,
                                   int capacity, int statusChoice, const string& extraValue, string& message) {
    LibraryResource* resource = resourceHashTable.find(Utils::toUpper(id));
    if (resource == nullptr) { message = "No resource with ID " + Utils::toUpper(id) + "."; return false; }
    LibraryResource* before = resource->clone();
    if (!name.empty())      resource->setName(Utils::sanitizeField(name));
    if (!location.empty())  resource->setLocation(Utils::sanitizeField(location));
    if (capacity >= 0)      resource->setCapacity(capacity);
    if (statusChoice == 1)  resource->setStatus(AvailabilityStatus::AVAILABLE);
    else if (statusChoice == 2) resource->setStatus(AvailabilityStatus::BORROWED);
    else if (statusChoice == 3) resource->setStatus(AvailabilityStatus::MAINTENANCE);
    if (!extraValue.empty()) resource->setExtraValue(Utils::sanitizeField(extraValue));
    pushUndo(GuiUndoType::UPDATED, before, 0);
    message = "Resource " + Utils::toUpper(id) + " updated.";
    return true;
}

bool GuiController::undo(string& message) {
    GuiUndoRecord record;
    if (!undoStack.pop(record)) { message = "Nothing to undo (the undo stack is empty)."; return false; }
    const string id = record.snapshot->getId();
    switch (record.type) {
        case GuiUndoType::ADDED: {
            int ignored = 0;
            LibraryResource* added = detachResource(id, ignored);
            delete added;
            delete record.snapshot;
            message = "Undo: the addition of " + id + " was reversed (resource removed).";
            break;
        }
        case GuiUndoType::DELETED: {
            if (insertResource(record.snapshot, record.position))
                message = "Undo: resource " + id + " restored at position " + to_string(record.position) + ".";
            else { delete record.snapshot; message = "Undo failed: the ID " + id + " is in use again."; }
            break;
        }
        case GuiUndoType::UPDATED: {
            LibraryResource* current = resourceHashTable.find(id);
            if (current != nullptr) current->assignFrom(*record.snapshot);
            delete record.snapshot;
            message = "Undo: the update of " + id + " was reversed.";
            break;
        }
    }
    return true;
}

// ---------------------------------------------------------------- search & sort
vector<ResourceRow> GuiController::searchByName(const string& keyword, int& examined) const {
    vector<ResourceRow> rows;
    examined = 0;
    const string needle = Utils::toLower(keyword);
    resourceList.forEach([&](LibraryResource* const& r) {
        ++examined;
        if (Utils::toLower(r->getName()).find(needle) != string::npos) rows.push_back(rowOf(r));
    });
    return rows;
}

vector<CompareRow> GuiController::compareSearch(const string& rawId, bool& found) const {
    const string id = Utils::toUpper(rawId);
    DynamicArray<LibraryResource*> all;
    resourceList.forEach([&](LibraryResource* const& r) { all.pushBack(r); });

    DynamicArray<LibraryResource*> sorted = all;
    Sorting::mergeSort(sorted, [](LibraryResource* a, LibraryResource* b) { return a->getId() < b->getId(); });

    int linearComparisons = 0, binaryComparisons = 0, treeComparisons = 0, hashComparisons = 0;
    int linearIndex = Searching::linearSearchById(all, id, linearComparisons);
    int binaryIndex = Searching::binarySearchById(sorted, id, binaryComparisons);
    LibraryResource* treeResult = resourceTree.search(id, &treeComparisons);
    LibraryResource* hashResult = resourceHashTable.find(id, &hashComparisons);

    found = (hashResult != nullptr);
    vector<CompareRow> rows;
    rows.push_back({"Linear search",     linearIndex >= 0, linearComparisons, "O(n)"});
    rows.push_back({"Binary search",     binaryIndex >= 0, binaryComparisons, "O(log n), needs sorted data"});
    rows.push_back({"AVL tree search",   treeResult != nullptr, treeComparisons, "O(log n) worst case"});
    rows.push_back({"Hash table lookup", hashResult != nullptr, hashComparisons, "O(1) average"});
    return rows;
}

vector<ResourceRow> GuiController::sortedResources(int field, int algo, long long& micros) const {
    DynamicArray<LibraryResource*> data;
    resourceList.forEach([&](LibraryResource* const& r) { data.pushBack(r); });

    auto less = [field](LibraryResource* a, LibraryResource* b) {
        switch (field) {
            case 1:  { string an = Utils::toLower(a->getName()), bn = Utils::toLower(b->getName());
                       return an != bn ? an < bn : a->getId() < b->getId(); }
            case 2:  return a->getCapacity() != b->getCapacity() ? a->getCapacity() < b->getCapacity()
                                                                 : a->getId() < b->getId();
            case 3:  { string at = a->getTypeName(), bt = b->getTypeName();
                       return at != bt ? at < bt : a->getId() < b->getId(); }
            default: return a->getId() < b->getId();
        }
    };

    long long start = microsNow();
    if (algo == 1) Sorting::insertionSort(data, less);
    else if (algo == 2) Sorting::mergeSort(data, less);
    else Sorting::quickSort(data, less);
    micros = microsNow() - start;

    vector<ResourceRow> rows;
    for (int i = 0; i < data.size(); ++i) rows.push_back(rowOf(data[i]));
    return rows;
}

// ---------------------------------------------------------------- requests
static RequestRow toRequestRow(const ResourceRequest& req) {
    RequestRow row;
    row.id = req.getRequestId();
    row.user = req.getUserName();
    row.resourceId = req.getRequestedResourceId();
    row.time = req.formattedTime();
    row.status = requestStatusToString(req.getStatus());
    row.priority = priorityToString(req.getPriority());
    row.priorityNum = static_cast<int>(req.getPriority());
    return row;
}

vector<RequestRow> GuiController::normalQueueRows() const {
    vector<RequestRow> rows;
    normalRequestQueue.forEach([&](const ResourceRequest& req) { rows.push_back(toRequestRow(req)); });
    return rows;
}

vector<RequestRow> GuiController::priorityRows() const {
    vector<RequestRow> rows;
    DynamicArray<ResourceRequest> sorted = priorityRequestHeap.toSortedArray();
    for (int i = 0; i < sorted.size(); ++i) rows.push_back(toRequestRow(sorted[i]));
    return rows;
}

vector<RequestRow> GuiController::historyRows() const {
    vector<RequestRow> rows;
    processedRequestHistory.forEach([&](const ResourceRequest& req) { rows.push_back(toRequestRow(req)); });
    return rows;
}

string GuiController::evaluate(ResourceRequest& request) {
    LibraryResource* resource = resourceHashTable.find(request.getRequestedResourceId());
    string reason;
    if (resource == nullptr) {
        request.setStatus(RequestStatus::REJECTED); reason = "resource does not exist";
    } else if (resource->getStatus() == AvailabilityStatus::MAINTENANCE) {
        request.setStatus(RequestStatus::REJECTED); reason = "resource is under maintenance";
    } else if (resource->getStatus() == AvailabilityStatus::AVAILABLE) {
        resource->setStatus(AvailabilityStatus::BORROWED);
        request.setStatus(RequestStatus::APPROVED);
        reason = "resource is now allocated to " + request.getUserName();
    } else if (request.getPriority() == PriorityLevel::EMERGENCY) {
        request.setStatus(RequestStatus::APPROVED);
        reason = "EMERGENCY override - approved ahead of the current holder";
    } else {
        request.setStatus(RequestStatus::REJECTED); reason = "resource is already borrowed";
    }
    processedRequestHistory.insertAtEnd(request);
    return "Request #" + to_string(request.getRequestId()) + " (" + request.getUserName() + " -> " +
           request.getRequestedResourceId() + ", " + priorityToString(request.getPriority()) + "): " +
           requestStatusToString(request.getStatus()) + " - " + reason + ".";
}

int GuiController::submitNormal(const string& user, const string& resourceId, string& message) {
    int id = nextRequestId++;
    ResourceRequest request(id, Utils::sanitizeField(user), Utils::toUpper(resourceId), time(nullptr),
                            RequestStatus::PENDING, PriorityLevel::NORMAL);
    normalRequestQueue.enqueue(request);
    message = "Normal request #" + to_string(id) + " queued (FIFO).";
    return id;
}

int GuiController::submitPriority(const string& user, const string& resourceId, PriorityLevel level,
                                  string& message) {
    int id = nextRequestId++;
    ResourceRequest request(id, Utils::sanitizeField(user), Utils::toUpper(resourceId), time(nullptr),
                            RequestStatus::PENDING, level);
    priorityRequestHeap.insert(request);
    message = priorityToString(level) + " request #" + to_string(id) + " added to the priority heap.";
    return id;
}

bool GuiController::processNextNormal(string& outcome) {
    ResourceRequest request;
    if (!normalRequestQueue.dequeue(request)) { outcome = "The normal queue is empty."; return false; }
    outcome = evaluate(request);
    return true;
}

int GuiController::processAllNormal(vector<string>& outcomes) {
    ResourceRequest request;
    int n = 0;
    while (normalRequestQueue.dequeue(request)) { outcomes.push_back(evaluate(request)); ++n; }
    return n;
}

bool GuiController::processNextPriority(string& outcome) {
    ResourceRequest request;
    if (!priorityRequestHeap.extractTop(request)) { outcome = "The priority heap is empty."; return false; }
    outcome = evaluate(request);
    return true;
}

int GuiController::processAllPriority(vector<string>& outcomes) {
    ResourceRequest request;
    int n = 0;
    while (priorityRequestHeap.extractTop(request)) { outcomes.push_back(evaluate(request)); ++n; }
    return n;
}

bool GuiController::cancelNormal(int requestId, string& message) {
    ResourceRequest removed;
    bool ok = normalRequestQueue.removeFirstMatching(
        [&](const ResourceRequest& req) { return req.getRequestId() == requestId; }, removed);
    if (!ok) { message = "No pending normal request #" + to_string(requestId) + "."; return false; }
    removed.setStatus(RequestStatus::CANCELLED);
    processedRequestHistory.insertAtEnd(removed);
    message = "Request #" + to_string(requestId) + " cancelled.";
    return true;
}

bool GuiController::returnResource(const string& rawId, string& message) {
    const string id = Utils::toUpper(rawId);
    LibraryResource* resource = resourceHashTable.find(id);
    if (resource == nullptr) { message = "No resource with ID " + id + "."; return false; }
    if (resource->getStatus() != AvailabilityStatus::BORROWED) {
        message = "Resource " + id + " is not currently borrowed."; return false;
    }
    resource->setStatus(AvailabilityStatus::AVAILABLE);
    message = "Resource " + id + " is available again.";
    return true;
}

// ---------------------------------------------------------------- AVL tree
vector<string> GuiController::traversal(int which) const {
    DynamicArray<LibraryResource*> out;
    if (which == 0) resourceTree.inorder(out);
    else if (which == 1) resourceTree.preorder(out);
    else if (which == 2) resourceTree.postorder(out);
    else resourceTree.levelOrder(out);
    vector<string> ids;
    for (int i = 0; i < out.size(); ++i) ids.push_back(out[i]->getId());
    return ids;
}

bool GuiController::treeSearch(const string& rawId, int& comparisons, ResourceRow& found) const {
    LibraryResource* r = resourceTree.search(Utils::toUpper(rawId), &comparisons);
    if (r == nullptr) return false;
    found = rowOf(r);
    return true;
}

// ---------------------------------------------------------------- hash table
string GuiController::hashStatistics() const {
    return captureCout([&] { resourceHashTable.printStatistics(); });
}

// ---------------------------------------------------------------- graph
vector<string> GuiController::locationList() const {
    vector<string> names;
    for (int i = 0; i < campusGraph.locationCount(); ++i) names.push_back(campusGraph.locationName(i));
    return names;
}

string GuiController::graphDescription() const {
    return captureCout([&] { campusGraph.display(); });
}

vector<string> GuiController::bfs(int start) const {
    DynamicArray<int> order;
    campusGraph.breadthFirstOrder(start, order);
    vector<string> names;
    for (int i = 0; i < order.size(); ++i) names.push_back(campusGraph.locationName(order[i]));
    return names;
}

vector<string> GuiController::dfs(int start) const {
    DynamicArray<int> order;
    campusGraph.depthFirstOrder(start, order);
    vector<string> names;
    for (int i = 0; i < order.size(); ++i) names.push_back(campusGraph.locationName(order[i]));
    return names;
}

bool GuiController::reachable(int fromIndex, int toIndex) const {
    return campusGraph.isReachable(fromIndex, toIndex);
}

int GuiController::shortestPath(int fromIndex, int toIndex, vector<string>& path) const {
    DynamicArray<int> indices;
    int metres = campusGraph.shortestPath(fromIndex, toIndex, indices);
    path.clear();
    for (int i = 0; i < indices.size(); ++i) path.push_back(campusGraph.locationName(indices[i]));
    return metres;
}

string GuiController::connectedComponents() const {
    return captureCout([&] { campusGraph.printConnectedComponents(); });
}

// ---------------------------------------------------------------- benchmark
vector<BenchRow> GuiController::runBenchmark() const {
    vector<BenchRow> results;
    const int sizes[] = {1000, 5000, 10000};
    for (int itemTotal : sizes) {
        unsigned int randomState = 12345;
        DynamicArray<int> idNumbers;
        for (int number = 1; number <= itemTotal; ++number) idNumbers.pushBack(number);
        for (int index = itemTotal - 1; index > 0; --index) {
            randomState = randomState * 1664525u + 1013904223u;
            int swapIndex = static_cast<int>((randomState >> 8) % static_cast<unsigned>(index + 1));
            Utils::swapValues(idNumbers[index], idNumbers[swapIndex]);
        }
        DynamicArray<LibraryResource*> dataset;
        for (int index = 0; index < itemTotal; ++index) {
            string idText = to_string(idNumbers[index]);
            idText = "B" + string(6 - idText.size(), '0') + idText;
            dataset.pushBack(new Book(idText, "Generated book " + idText, "Stack", 1,
                                      AvailabilityStatus::AVAILABLE, "Author"));
        }
        auto byIdLess = [](LibraryResource* a, LibraryResource* b) { return a->getId() < b->getId(); };

        DynamicArray<LibraryResource*> insertionData = dataset, mergeData = dataset, quickData = dataset;
        long long start = microsNow();
        Sorting::insertionSort(insertionData, byIdLess);
        long long insertionTime = microsNow() - start;
        start = microsNow(); Sorting::mergeSort(mergeData, byIdLess);
        long long mergeTime = microsNow() - start;
        start = microsNow(); Sorting::quickSort(quickData, byIdLess);
        long long quickTime = microsNow() - start;

        ResourceBST benchmarkTree;
        ResourceHashTable benchmarkTable(itemTotal / 2 + 1);
        for (int index = 0; index < itemTotal; ++index) {
            benchmarkTree.insert(dataset[index]);
            benchmarkTable.insert(dataset[index]);
        }
        const int lookups = 1000;
        DynamicArray<string> queryIds;
        for (int query = 0; query < lookups; ++query) queryIds.pushBack(dataset[(query * 37) % itemTotal]->getId());

        volatile long long checksum = 0;
        int comparisons = 0;
        start = microsNow();
        for (int q = 0; q < lookups; ++q) checksum = checksum + Searching::linearSearchById(dataset, queryIds[q], comparisons);
        long long linearTime = microsNow() - start;
        start = microsNow();
        for (int q = 0; q < lookups; ++q) checksum = checksum + Searching::binarySearchById(mergeData, queryIds[q], comparisons);
        long long binaryTime = microsNow() - start;
        start = microsNow();
        for (int q = 0; q < lookups; ++q) checksum = checksum + (benchmarkTree.search(queryIds[q]) != nullptr ? 1 : 0);
        long long treeTime = microsNow() - start;
        start = microsNow();
        for (int q = 0; q < lookups; ++q) checksum = checksum + (benchmarkTable.find(queryIds[q]) != nullptr ? 1 : 0);
        long long hashTime = microsNow() - start;
        (void)checksum;

        BenchRow row;
        row.n = itemTotal; row.insertion = insertionTime; row.merge = mergeTime; row.quick = quickTime;
        row.linear = linearTime; row.binary = binaryTime; row.bst = treeTime; row.hash = hashTime;
        results.push_back(row);

        for (int index = 0; index < dataset.size(); ++index) delete dataset[index];
    }
    return results;
}
