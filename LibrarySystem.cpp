#include "LibrarySystem.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <cmath>
#include "Utils.h"

DynamicArray<LibraryResource*> LibrarySystem::collectResources() const {
    DynamicArray<LibraryResource*> collected;
    resourceList.forEach([&collected](LibraryResource* const& resource) { collected.pushBack(resource); });
    return collected;
}

bool LibrarySystem::insertResource(LibraryResource* resource, int position) {
    if (resourceHashTable.find(resource->getId()) != nullptr) return false;
    if (position < 0 || position > resourceList.size()) position = resourceList.size();
    resourceList.insertAtPosition(position, resource);
    resourceTree.insert(resource);
    resourceHashTable.insert(resource);
    return true;
}

LibraryResource* LibrarySystem::detachResource(const string& id, int& oldPosition) {
    oldPosition = resourceList.findIndex(
        [&id](LibraryResource* const& resource) { return resource->getId() == id; });
    if (oldPosition == -1) return nullptr;
    LibraryResource* detached = nullptr;
    resourceList.removeAtPosition(oldPosition, detached);
    resourceTree.remove(id);
    resourceHashTable.remove(id);
    return detached;
}

void LibrarySystem::pushUndo(UndoActionType type, LibraryResource* snapshot, int position) {
    UndoRecord record;
    record.actionType = type;
    record.resourceSnapshot = snapshot;
    record.originalPosition = position;
    undoStack.push(record);
}

string LibrarySystem::describeUndoRecord(const UndoRecord& record) {
    const string& id = record.resourceSnapshot->getId();
    switch (record.actionType) {
        case UndoActionType::ADDED_RESOURCE:   return "Added resource " + id + "   (undo = remove it)";
        case UndoActionType::DELETED_RESOURCE: return "Deleted resource " + id + " (undo = restore it)";
        case UndoActionType::UPDATED_RESOURCE: return "Updated resource " + id + " (undo = restore old values)";
    }
    return "";
}

AvailabilityStatus LibrarySystem::promptForStatus() const {
    cout << "  1. Available   2. Borrowed   3. Maintenance\n";
    int choice = Utils::readInt("  Status: ", 1, 3);
    if (choice == 2) return AvailabilityStatus::BORROWED;
    if (choice == 3) return AvailabilityStatus::MAINTENANCE;
    return AvailabilityStatus::AVAILABLE;
}

LibraryResource* LibrarySystem::promptForNewResource() const {
    cout << "  Resource type:  1. Book   2. Study room   3. Equipment\n";
    int typeChoice = Utils::readInt("  Type: ", 1, 3);
    string typeCode = (typeChoice == 1) ? "BOOK" : (typeChoice == 2) ? "ROOM" : "EQUIPMENT";

    string id = Utils::toUpper(Utils::readNonEmpty("  Resource ID (e.g. B100): "));
    if (resourceHashTable.find(id) != nullptr) {
        cout << "  Error: a resource with ID " << id << " already exists.\n";
        return nullptr;
    }
    string name = Utils::readNonEmpty("  Name: ");
    string location = Utils::readNonEmpty("  Location (shelf / room): ");
    string capacityPrompt = (typeChoice == 1) ? "  Number of copies: "
                                : (typeChoice == 2) ? "  Number of seats: " : "  Number of units: ";
    int capacity = Utils::readInt(capacityPrompt, 0, 100000);
    AvailabilityStatus status = promptForStatus();
    string extraPrompt = (typeChoice == 1) ? "  Author: "
                             : (typeChoice == 2) ? "  Floor number: " : "  Condition note: ";
    string extraValue = Utils::readNonEmpty(extraPrompt);
    return ResourceFactory::create(typeCode, id, name, location, capacity, status, extraValue);
}

void LibrarySystem::printResourceTable(const DynamicArray<LibraryResource*>& resources) const {
    LibraryResource::printTableHeader();
    for (int index = 0; index < resources.size(); ++index) resources[index]->printRow();
    cout << "  (" << resources.size() << " resource(s))\n";
}

void LibrarySystem::printPathOfLocations(const DynamicArray<int>& order, const string& separator) const {
    cout << "  ";
    for (int index = 0; index < order.size(); ++index) {
        cout << campusGraph.locationName(order[index]) << (index + 1 < order.size() ? separator : "");
    }
    cout << "\n";
}

int LibrarySystem::chooseLocation(const string& prompt) const {
    if (campusGraph.locationCount() == 0) {
        cout << "  There are no locations yet.\n";
        return -1;
    }
    for (int index = 0; index < campusGraph.locationCount(); ++index) {
        cout << "   " << (index + 1) << ". " << campusGraph.locationName(index) << "\n";
    }
    return Utils::readInt(prompt, 1, campusGraph.locationCount()) - 1;
}

long long LibrarySystem::currentMicroseconds() {
    return static_cast<long long>(clock()) * 1000000LL / CLOCKS_PER_SEC;
}

void LibrarySystem::addResource(InsertMode mode) {
    LibraryResource* newResource = promptForNewResource();
    if (newResource == nullptr) return;
    int position = 0;
    if (mode == InsertMode::END) position = resourceList.size();
    else if (mode == InsertMode::POSITION) {
        position = Utils::readInt("  Position (0 = first, " + to_string(resourceList.size()) + " = last): ",
                                  0, resourceList.size());
    }
    insertResource(newResource, position);
    pushUndo(UndoActionType::ADDED_RESOURCE, newResource->clone(), position);
    cout << "  Resource " << newResource->getId() << " added at list position " << position << ".\n";
}

void LibrarySystem::removeResource() {
    string id = Utils::toUpper(Utils::readLine("  Resource ID to remove: "));
    int oldPosition = 0;
    LibraryResource* detached = detachResource(id, oldPosition);
    if (detached == nullptr) {
        cout << "  Error: no resource with ID " << id << ".\n";
        return;
    }
    // The detached object itself becomes the snapshot, so UNDO can restore it.
    pushUndo(UndoActionType::DELETED_RESOURCE, detached, oldPosition);
    cout << "  Resource " << id << " removed (use Undo to restore it).\n";
}

void LibrarySystem::updateResource() {
    string id = Utils::toUpper(Utils::readLine("  Resource ID to update: "));
    LibraryResource* resource = resourceHashTable.find(id);
    if (resource == nullptr) {
        cout << "  Error: no resource with ID " << id << ".\n";
        return;
    }
    LibraryResource::printTableHeader();
    resource->printRow();
    cout << "  (press Enter to keep the current value)\n";

    LibraryResource* before = resource->clone();

    string text = Utils::sanitizeField(Utils::readLine("  New name [" + resource->getName() + "]: "));
    if (!text.empty()) resource->setName(text);

    text = Utils::sanitizeField(Utils::readLine("  New location [" + resource->getLocation() + "]: "));
    if (!text.empty()) resource->setLocation(text);

    text = Utils::readLine("  New capacity [" + to_string(resource->getCapacity()) + "]: ");
    int newCapacity = 0;
    if (!text.empty()) {
        if (Utils::tryParseInt(text, 0, 100000, newCapacity)) resource->setCapacity(newCapacity);
        else cout << "  (invalid capacity ignored)\n";
    }

    text = Utils::readLine("  New status 1=Available 2=Borrowed 3=Maintenance [" +
                           statusToString(resource->getStatus()) + "]: ");
    int statusChoice = 0;
    if (!text.empty()) {
        if (Utils::tryParseInt(text, 1, 3, statusChoice)) {
            resource->setStatus(statusChoice == 1 ? AvailabilityStatus::AVAILABLE
    : statusChoice == 2 ? AvailabilityStatus::BORROWED
    : AvailabilityStatus::MAINTENANCE);
        } else {
            cout << "  (invalid status ignored)\n";
        }
    }

    text = Utils::sanitizeField(Utils::readLine("  New " + resource->getExtraLabel() + " [" +
                                                resource->getExtraValue() + "]: "));
    if (!text.empty()) resource->setExtraValue(text);

    pushUndo(UndoActionType::UPDATED_RESOURCE, before, 0);
    cout << "  Resource " << id << " updated.\n";
}

void LibrarySystem::displayAllResources() const {
    if (resourceList.isEmpty()) {
        cout << "  The catalogue is empty.\n";
        return;
    }
    printResourceTable(collectResources());
}

void LibrarySystem::resourceManagementMenu() {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("1. RESOURCE MANAGEMENT (linked list)");
        cout << "  1. Add resource at the BEGINNING of the list\n"
                     "  2. Add resource at the END of the list\n"
                     "  3. Add resource at a chosen POSITION\n"
                     "  4. Remove a resource\n"
                     "  5. Update a resource\n"
                     "  6. Display all resources (list traversal)\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 6)) {
            case 1: addResource(InsertMode::BEGINNING); break;
            case 2: addResource(InsertMode::END); break;
            case 3: addResource(InsertMode::POSITION); break;
            case 4: removeResource(); break;
            case 5: updateResource(); break;
            case 6: displayAllResources(); break;
            case 0: stayInMenu = false; break;
        }
    }
}

bool LibrarySystem::sortedByIdLess(LibraryResource* first, LibraryResource* second) {
    return first->getId() < second->getId();
}

void LibrarySystem::linearSearchByName() const {
    string keyword = Utils::toLower(Utils::readNonEmpty("  Name contains: "));
    DynamicArray<LibraryResource*> matches;
    int comparisons = 0;
    resourceList.forEach([&](LibraryResource* const& resource) {
        ++comparisons;
        if (Utils::toLower(resource->getName()).find(keyword) != string::npos) matches.pushBack(resource);
    });
    if (matches.isEmpty()) cout << "  No resource name contains \"" << keyword << "\".\n";
    else printResourceTable(matches);
    cout << "  Linear search examined " << comparisons << " resource(s)  [O(n)].\n";
}

void LibrarySystem::binarySearchById() const {
    string id = Utils::toUpper(Utils::readLine("  Resource ID: "));
    DynamicArray<LibraryResource*> sortedResources = collectResources();
    Sorting::mergeSort(sortedResources, sortedByIdLess);   // binary search needs sorted data!
    int comparisons = 0;
    int index = Searching::binarySearchById(sortedResources, id, comparisons);
    if (index == -1) cout << "  Resource " << id << " was not found.\n";
    else {
        LibraryResource::printTableHeader();
        sortedResources[index]->printRow();
    }
    cout << "  Binary search needed " << comparisons << " comparison(s)  [O(log n)] after sorting by ID.\n";
}

void LibrarySystem::compareSearchMethods() const {
    string id = Utils::toUpper(Utils::readLine("  Resource ID to look for: "));
    DynamicArray<LibraryResource*> unsorted = collectResources();
    DynamicArray<LibraryResource*> sorted = unsorted;
    Sorting::mergeSort(sorted, sortedByIdLess);

    int linearComparisons = 0, binaryComparisons = 0, treeComparisons = 0, hashComparisons = 0;
    long long start = currentMicroseconds();
    int linearIndex = Searching::linearSearchById(unsorted, id, linearComparisons);
    long long linearTime = currentMicroseconds() - start;

    start = currentMicroseconds();
    int binaryIndex = Searching::binarySearchById(sorted, id, binaryComparisons);
    long long binaryTime = currentMicroseconds() - start;

    start = currentMicroseconds();
    LibraryResource* treeResult = resourceTree.search(id, &treeComparisons);
    long long treeTime = currentMicroseconds() - start;

    start = currentMicroseconds();
    LibraryResource* hashResult = resourceHashTable.find(id, &hashComparisons);
    long long hashTime = currentMicroseconds() - start;

    cout << "\n  " << left << setw(22) << "Method" << setw(8) << "Found" << setw(14)
              << "Comparisons" << setw(12) << "Time (us)" << "Complexity\n";
    Utils::printLine('-', 75);
    cout << "  " << left << setw(22) << "Linear search" << setw(8) << (linearIndex >= 0 ? "yes" : "no")
              << setw(14) << linearComparisons << setw(12) << linearTime << "O(n)\n";
    cout << "  " << left << setw(22) << "Binary search" << setw(8) << (binaryIndex >= 0 ? "yes" : "no")
              << setw(14) << binaryComparisons << setw(12) << binaryTime << "O(log n), data must be sorted\n";
    cout << "  " << left << setw(22) << "BST search" << setw(8) << (treeResult ? "yes" : "no")
              << setw(14) << treeComparisons << setw(12) << treeTime << "O(log n) worst case (AVL)\n";
    cout << "  " << left << setw(22) << "Hash table lookup" << setw(8) << (hashResult ? "yes" : "no")
              << setw(14) << hashComparisons << setw(12) << hashTime << "O(1) average\n";
    cout << "  (Timings on a tiny catalogue are close to zero - use option 10 for large datasets.)\n";
}

void LibrarySystem::searchMenu() const {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("2. SEARCH RESOURCES");
        cout << "  1. Linear search by name (keyword)\n"
                     "  2. Binary search by ID (sorts first)\n"
                     "  3. Compare linear / binary / BST / hash for one ID\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 3)) {
            case 1: linearSearchByName(); break;
            case 2: binarySearchById(); break;
            case 3: compareSearchMethods(); break;
            case 0: stayInMenu = false; break;
        }
    }
}

string LibrarySystem::algorithmName(int algorithmChoice) {
    return algorithmChoice == 1 ? "Insertion sort  O(n^2)"
    : algorithmChoice == 2 ? "Merge sort  O(n log n)" : "Quick sort  O(n log n) average";
}

void LibrarySystem::sortResources() const {
    if (resourceList.isEmpty()) {
        cout << "  Nothing to sort.\n";
        return;
    }
    cout << "  Sort by:  1. Resource ID   2. Name   3. Capacity   4. Type\n";
    int criterion = Utils::readInt("  Criterion: ", 1, 4);
    cout << "  Algorithm:  1. Insertion sort   2. Merge sort   3. Quick sort\n";
    int algorithm = Utils::readInt("  Algorithm: ", 1, 3);

    DynamicArray<LibraryResource*> data = collectResources();
    long long elapsed = 0;
    // Every comparator breaks ties with the ID so the order is always well defined.
    if (criterion == 1) {
        elapsed = runSortingAlgorithm(algorithm, data, [](LibraryResource* a, LibraryResource* b) {
            return a->getId() < b->getId(); });
    } else if (criterion == 2) {
        elapsed = runSortingAlgorithm(algorithm, data, [](LibraryResource* a, LibraryResource* b) {
            string nameA = Utils::toLower(a->getName()), nameB = Utils::toLower(b->getName());
            return nameA != nameB ? nameA < nameB : a->getId() < b->getId(); });
    } else if (criterion == 3) {
        elapsed = runSortingAlgorithm(algorithm, data, [](LibraryResource* a, LibraryResource* b) {
            return a->getCapacity() != b->getCapacity() ? a->getCapacity() < b->getCapacity() : a->getId() < b->getId(); });
    } else {
        elapsed = runSortingAlgorithm(algorithm, data, [](LibraryResource* a, LibraryResource* b) {
            return a->getTypeName() != b->getTypeName() ? a->getTypeName() < b->getTypeName() : a->getId() < b->getId(); });
    }
    printResourceTable(data);
    cout << "  " << algorithmName(algorithm) << " took " << elapsed << " microsecond(s).\n";
}

void LibrarySystem::sortRequests() const {
    DynamicArray<ResourceRequest> allRequests;
    normalRequestQueue.forEach([&allRequests](const ResourceRequest& request) { allRequests.pushBack(request); });
    priorityRequestHeap.copyItemsTo(allRequests);
    processedRequestHistory.forEach([&allRequests](const ResourceRequest& request) { allRequests.pushBack(request); });
    if (allRequests.isEmpty()) {
        cout << "  There are no requests to sort.\n";
        return;
    }
    cout << "  Sort requests by:  1. Submission time   2. Priority   3. Status\n";
    int criterion = Utils::readInt("  Criterion: ", 1, 3);
    cout << "  Algorithm:  1. Insertion sort   2. Merge sort   3. Quick sort\n";
    int algorithm = Utils::readInt("  Algorithm: ", 1, 3);

    long long elapsed = 0;
    if (criterion == 1) {
        elapsed = runSortingAlgorithm(algorithm, allRequests, [](const ResourceRequest& a, const ResourceRequest& b) {
            return a.getRequestId() < b.getRequestId(); });   // IDs grow with time
    } else if (criterion == 2) {
        elapsed = runSortingAlgorithm(algorithm, allRequests, [](const ResourceRequest& a, const ResourceRequest& b) {
            return a < b; });
    } else {
        elapsed = runSortingAlgorithm(algorithm, allRequests, [](const ResourceRequest& a, const ResourceRequest& b) {
            return a.getStatus() != b.getStatus() ? static_cast<int>(a.getStatus()) < static_cast<int>(b.getStatus())
    : a.getRequestId() < b.getRequestId(); });
    }
    ResourceRequest::printTableHeader();
    for (int index = 0; index < allRequests.size(); ++index) allRequests[index].printRow();
    cout << "  " << algorithmName(algorithm) << " took " << elapsed << " microsecond(s).\n";
}

void LibrarySystem::sortMenu() const {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("3. SORT");
        cout << "  1. Sort resources\n"
                     "  2. Sort requests\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 2)) {
            case 1: sortResources(); break;
            case 2: sortRequests(); break;
            case 0: stayInMenu = false; break;
        }
    }
}

void LibrarySystem::evaluateRequest(ResourceRequest& request) {
    LibraryResource* resource = resourceHashTable.find(request.getRequestedResourceId());
    string reason;
    if (resource == nullptr) {
        request.setStatus(RequestStatus::REJECTED);
        reason = "resource does not exist";
    } else if (resource->getStatus() == AvailabilityStatus::MAINTENANCE) {
        request.setStatus(RequestStatus::REJECTED);
        reason = "resource is under maintenance";
    } else if (resource->getStatus() == AvailabilityStatus::AVAILABLE) {
        resource->setStatus(AvailabilityStatus::BORROWED);
        request.setStatus(RequestStatus::APPROVED);
        reason = "resource is now allocated to " + request.getUserName();
    } else if (request.getPriority() == PriorityLevel::EMERGENCY) {
        // Priority rule: an EMERGENCY request may take a resource that is already borrowed.
        request.setStatus(RequestStatus::APPROVED);
        reason = "EMERGENCY override - resource reassigned to " + request.getUserName();
    } else {
        request.setStatus(RequestStatus::REJECTED);
        reason = "resource is already borrowed";
    }
    cout << "  Request #" << request.getRequestId() << " (" << request.getUserName() << " -> "
              << request.getRequestedResourceId() << ", " << priorityToString(request.getPriority()) << "): "
              << requestStatusToString(request.getStatus()) << " - " << reason << ".\n";
    processedRequestHistory.insertAtEnd(request);
}

bool LibrarySystem::createRequest(PriorityLevel priority, ResourceRequest& newRequest) {
    string user = Utils::readNonEmpty("  Your name: ");
    string resourceId = Utils::toUpper(Utils::readNonEmpty("  Resource ID requested: "));
    if (resourceHashTable.find(resourceId) == nullptr) {
        cout << "  Error: resource " << resourceId << " does not exist. Request not submitted.\n";
        return false;
    }
    newRequest = ResourceRequest(nextRequestId++, user, resourceId, time(nullptr),
                                 RequestStatus::PENDING, priority);
    return true;
}

void LibrarySystem::submitNormalRequest() {
    ResourceRequest request;
    if (!createRequest(PriorityLevel::NORMAL, request)) return;
    normalRequestQueue.enqueue(request);
    cout << "  Request #" << request.getRequestId() << " joined the FIFO queue (position "
              << normalRequestQueue.size() << ").\n";
}

void LibrarySystem::viewPendingNormalRequests() const {
    if (normalRequestQueue.isEmpty()) {
        cout << "  The request queue is empty.\n";
        return;
    }
    ResourceRequest::printTableHeader();
    normalRequestQueue.forEach([](const ResourceRequest& request) { request.printRow(); });
    cout << "  (front of the queue is the first row)\n";
}

void LibrarySystem::processNextNormalRequest() {
    ResourceRequest request;
    if (!normalRequestQueue.dequeue(request)) {
        cout << "  The request queue is empty - nothing to process.\n";
        return;
    }
    evaluateRequest(request);
}

void LibrarySystem::processAllNormalRequests() {
    if (normalRequestQueue.isEmpty()) {
        cout << "  The request queue is empty - nothing to process.\n";
        return;
    }
    ResourceRequest request;
    while (normalRequestQueue.dequeue(request)) evaluateRequest(request);
}

void LibrarySystem::cancelNormalRequest() {
    int requestId = Utils::readInt("  Request ID to cancel: ", 1, 1000000);
    ResourceRequest removed;
    if (normalRequestQueue.removeFirstMatching(
            [requestId](const ResourceRequest& request) { return request.getRequestId() == requestId; }, removed)) {
        removed.setStatus(RequestStatus::CANCELLED);
        processedRequestHistory.insertAtEnd(removed);
        cout << "  Request #" << requestId << " cancelled.\n";
    } else {
        cout << "  Request #" << requestId << " is not in the queue.\n";
    }
}

void LibrarySystem::returnResource() {
    string id = Utils::toUpper(Utils::readLine("  Resource ID being returned: "));
    LibraryResource* resource = resourceHashTable.find(id);
    if (resource == nullptr) cout << "  Error: no resource with ID " << id << ".\n";
    else if (resource->getStatus() != AvailabilityStatus::BORROWED) cout << "  That resource is not currently borrowed.\n";
    else {
        resource->setStatus(AvailabilityStatus::AVAILABLE);
        cout << "  Resource " << id << " is available again.\n";
    }
}

void LibrarySystem::viewRequestHistory() const {
    if (processedRequestHistory.isEmpty()) {
        cout << "  No request has been processed yet.\n";
        return;
    }
    ResourceRequest::printTableHeader();
    processedRequestHistory.forEach([](const ResourceRequest& request) { request.printRow(); });
}

void LibrarySystem::requestMenu() {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("4. RESOURCE REQUESTS (FIFO queue)");
        cout << "  1. Submit a request (enqueue)\n"
                     "  2. View pending requests\n"
                     "  3. Process the next request (dequeue)\n"
                     "  4. Process ALL pending requests in FIFO order\n"
                     "  5. Cancel a pending request\n"
                     "  6. Return a borrowed resource\n"
                     "  7. View processed request history\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 7)) {
            case 1: submitNormalRequest(); break;
            case 2: viewPendingNormalRequests(); break;
            case 3: processNextNormalRequest(); break;
            case 4: processAllNormalRequests(); break;
            case 5: cancelNormalRequest(); break;
            case 6: returnResource(); break;
            case 7: viewRequestHistory(); break;
            case 0: stayInMenu = false; break;
        }
    }
}

void LibrarySystem::submitPriorityRequest() {
    cout << "  Priority:  1. Emergency   2. High   3. Normal   4. Low\n";
    int choice = Utils::readInt("  Priority: ", 1, 4);
    ResourceRequest request;
    if (!createRequest(static_cast<PriorityLevel>(choice), request)) return;
    priorityRequestHeap.insert(request);
    cout << "  Request #" << request.getRequestId() << " inserted into the priority heap ("
              << priorityToString(request.getPriority()) << ").\n";
}

void LibrarySystem::viewPriorityRequests() const {
    if (priorityRequestHeap.isEmpty()) {
        cout << "  The priority heap is empty.\n";
        return;
    }
    DynamicArray<ResourceRequest> ordered = priorityRequestHeap.toSortedArray();
    ResourceRequest::printTableHeader();
    for (int index = 0; index < ordered.size(); ++index) ordered[index].printRow();
    cout << "  (listed in the order they will be processed)\n";
}

void LibrarySystem::peekHighestPriority() const {
    ResourceRequest topRequest;
    if (!priorityRequestHeap.peekTop(topRequest)) {
        cout << "  The priority heap is empty.\n";
        return;
    }
    cout << "  Next request to be processed (heap root):\n";
    ResourceRequest::printTableHeader();
    topRequest.printRow();
}

void LibrarySystem::processHighestPriorityRequest() {
    ResourceRequest request;
    if (!priorityRequestHeap.extractTop(request)) {
        cout << "  The priority heap is empty - nothing to process.\n";
        return;
    }
    evaluateRequest(request);
}

void LibrarySystem::processAllPriorityRequests() {
    if (priorityRequestHeap.isEmpty()) {
        cout << "  The priority heap is empty - nothing to process.\n";
        return;
    }
    ResourceRequest request;
    while (priorityRequestHeap.extractTop(request)) evaluateRequest(request);
}

void LibrarySystem::priorityMenu() {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("5. PRIORITY REQUESTS (binary heap)");
        cout << "  1. Submit a priority request (heap insert)\n"
                     "  2. View pending requests in processing order\n"
                     "  3. Peek at the highest priority request\n"
                     "  4. Process the highest priority request (extract-min)\n"
                     "  5. Process ALL requests by priority\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 5)) {
            case 1: submitPriorityRequest(); break;
            case 2: viewPriorityRequests(); break;
            case 3: peekHighestPriority(); break;
            case 4: processHighestPriorityRequest(); break;
            case 5: processAllPriorityRequests(); break;
            case 0: stayInMenu = false; break;
        }
    }
}

void LibrarySystem::undoLastOperation() {
    UndoRecord record;
    if (!undoStack.pop(record)) {
        cout << "  Nothing to undo (the undo stack is empty).\n";
        return;
    }
    string id = record.resourceSnapshot->getId();
    switch (record.actionType) {
        case UndoActionType::ADDED_RESOURCE: {
            int ignoredPosition = 0;
            LibraryResource* addedResource = detachResource(id, ignoredPosition);
            delete addedResource;
            delete record.resourceSnapshot;
            cout << "  Undo: the addition of " << id << " was reversed (resource removed).\n";
            break;
        }
        case UndoActionType::DELETED_RESOURCE: {
            // Ownership of the snapshot goes back into the catalogue.
            if (insertResource(record.resourceSnapshot, record.originalPosition)) {
                cout << "  Undo: resource " << id << " restored at list position "
                          << record.originalPosition << ".\n";
            } else {
                delete record.resourceSnapshot;
                cout << "  Undo failed: the ID " << id << " is in use again.\n";
            }
            break;
        }
        case UndoActionType::UPDATED_RESOURCE: {
            LibraryResource* current = resourceHashTable.find(id);
            if (current != nullptr) current->assignFrom(*record.resourceSnapshot);
            delete record.resourceSnapshot;
            cout << "  Undo: the update of " << id << " was reversed.\n";
            break;
        }
    }
}

void LibrarySystem::undoMenu() {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("6. UNDO OPERATIONS (stack)");
        cout << "  Stack size: " << undoStack.size() << "\n"
                     "  1. Undo the last operation (pop)\n"
                     "  2. Peek at the last operation (top)\n"
                     "  3. Show the whole undo history (top first)\n"
                     "  0. Back\n";
        switch (Utils::readInt("Enter your choice: ", 0, 3)) {
            case 1: undoLastOperation(); break;
            case 2: {
                UndoRecord top;
                if (undoStack.peek(top)) cout << "  Top of stack: " << describeUndoRecord(top) << "\n";
                else cout << "  The undo stack is empty.\n";
                break;
            }
            case 3:
                if (undoStack.isEmpty()) cout << "  The undo stack is empty.\n";
                else {
                    int number = 1;
                    undoStack.forEach([&number](const UndoRecord& record) {
                        cout << "  " << number++ << ". " << describeUndoRecord(record) << "\n";
                    });
                }
                break;
            case 0: stayInMenu = false; break;
        }
    }
}

void LibrarySystem::printIdSequence(const string& title, const DynamicArray<LibraryResource*>& sequence) const {
    cout << "  " << left << setw(12) << title << ": ";
    for (int index = 0; index < sequence.size(); ++index) cout << sequence[index]->getId() << " ";
    cout << "\n";
}

void LibrarySystem::treeMenu() const {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("7. TREE OPERATIONS (binary search tree on Resource ID)");
        cout << "  1. Show all four traversals\n"
                     "  2. Search the tree for an ID\n"
                     "  3. Tree height and size\n"
                     "  0. Back\n";
        int choice = Utils::readInt("Enter your choice: ", 0, 3);
        if (choice == 1) {
            if (resourceTree.isEmpty()) {
                cout << "  The tree is empty.\n";
                continue;
            }
            DynamicArray<LibraryResource*> inorderIds, preorderIds, postorderIds, levelIds;
            resourceTree.inorder(inorderIds);
            resourceTree.preorder(preorderIds);
            resourceTree.postorder(postorderIds);
            resourceTree.levelOrder(levelIds);
            printIdSequence("Inorder", inorderIds);
            cout << "                (inorder gives IDs in sorted order)\n";
            printIdSequence("Preorder", preorderIds);
            printIdSequence("Postorder", postorderIds);
            printIdSequence("Level-order", levelIds);
        } else if (choice == 2) {
            string id = Utils::toUpper(Utils::readLine("  Resource ID: "));
            int comparisons = 0;
            LibraryResource* found = resourceTree.search(id, &comparisons);
            if (found == nullptr) cout << "  " << id << " is not in the tree (" << comparisons << " comparison(s)).\n";
            else {
                LibraryResource::printTableHeader();
                found->printRow();
                cout << "  Found after " << comparisons << " comparison(s).\n";
            }
        } else if (choice == 3) {
            cout << "  Nodes : " << resourceTree.size() << "\n  Height: " << resourceTree.height() << "\n"
                      << "  (AVL balancing keeps the height close to the minimum possible, about "
                      << static_cast<int>(ceil(log2(resourceTree.size() + 1.0))) << ")\n";
        } else {
            stayInMenu = false;
        }
    }
}

void LibrarySystem::hashMenu() const {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("8. HASH TABLE LOOKUP (separate chaining)");
        cout << "  1. Look up a resource by ID\n"
                     "  2. Show hash table statistics\n"
                     "  3. Show non-empty buckets\n"
                     "  0. Back\n";
        int choice = Utils::readInt("Enter your choice: ", 0, 3);
        if (choice == 1) {
            string id = Utils::toUpper(Utils::readLine("  Resource ID: "));
            int comparisons = 0;
            LibraryResource* found = resourceHashTable.find(id, &comparisons);
            if (found == nullptr) cout << "  Resource " << id << " not found (" << comparisons << " comparison(s)).\n";
            else {
                LibraryResource::printTableHeader();
                found->printRow();
                cout << "  Retrieved with " << comparisons << " key comparison(s) inside one bucket.\n";
            }
        } else if (choice == 2) {
            resourceHashTable.printStatistics();
        } else if (choice == 3) {
            resourceHashTable.printBuckets();
        } else {
            stayInMenu = false;
        }
    }
}

void LibrarySystem::navigationMenu() {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("9. CAMPUS NAVIGATION (graph)");
        cout << "  1. Add a location\n"
                     "  2. Add a pathway between two locations\n"
                     "  3. Display all locations and pathways\n"
                     "  4. Breadth-first traversal (BFS)\n"
                     "  5. Depth-first traversal (DFS)\n"
                     "  6. Is location B reachable from location A?\n"
                     "  7. Shortest walking path (Dijkstra)\n"
                     "  8. Connected components\n"
                     "  0. Back\n";
        int choice = Utils::readInt("Enter your choice: ", 0, 8);
        if (choice == 0) {
            stayInMenu = false;
        } else if (choice == 1) {
            string name = Utils::readNonEmpty("  New location name: ");
            if (campusGraph.addLocation(name)) cout << "  Location \"" << name << "\" added.\n";
            else cout << "  That location already exists.\n";
        } else if (choice == 2) {
            cout << "  First location:\n";
            int first = chooseLocation("  Number: ");
            if (first < 0) continue;
            cout << "  Second location:\n";
            int second = chooseLocation("  Number: ");
            int meters = Utils::readInt("  Distance in metres: ", 1, 100000);
            if (campusGraph.addConnection(first, second, meters)) cout << "  Pathway added.\n";
            else cout << "  Could not add the pathway (same location or it already exists).\n";
        } else if (choice == 3) {
            campusGraph.display();
        } else if (choice == 4 || choice == 5) {
            int start = chooseLocation("  Start location number: ");
            if (start < 0) continue;
            DynamicArray<int> order;
            if (choice == 4) campusGraph.breadthFirstOrder(start, order);
            else campusGraph.depthFirstOrder(start, order);
            cout << (choice == 4 ? "  BFS" : "  DFS") << " visit order (" << order.size() << " location(s)):\n";
            printPathOfLocations(order, " -> ");
            cout << (choice == 4
                ? "  BFS explores nearest locations first (queue), so it finds the path with the fewest hops.\n"
                : "  DFS follows one route as deep as possible before backtracking (recursion / stack).\n");
        } else if (choice == 6 || choice == 7) {
            cout << "  From:\n";
            int from = chooseLocation("  Number: ");
            if (from < 0) continue;
            cout << "  To:\n";
            int to = chooseLocation("  Number: ");
            if (choice == 6) {
                cout << "  " << campusGraph.locationName(to)
                          << (campusGraph.isReachable(from, to) ? " IS reachable from " : " is NOT reachable from ")
                          << campusGraph.locationName(from) << ".\n";
            } else {
                DynamicArray<int> path;
                int meters = campusGraph.shortestPath(from, to, path);
                if (meters < 0) cout << "  No path exists between these locations.\n";
                else {
                    cout << "  Shortest path (" << meters << " metres):\n";
                    printPathOfLocations(path, " -> ");
                }
            }
        } else if (choice == 8) {
            if (campusGraph.locationCount() == 0) cout << "  The graph is empty.\n";
            else cout << "  Total connected components: " << campusGraph.printConnectedComponents() << "\n";
        }
    }
}

void LibrarySystem::printComplexityTable() const {
    cout << "\n  " << left << setw(28) << "Operation" << setw(26) << "Structure / algorithm"
              << setw(22) << "Complexity" << "Case\n";
    Utils::printLine('-', 92);
    struct Row { const char* operation; const char* structure; const char* complexity; const char* caseType; };
    const Row rows[] = {
        {"Enqueue / dequeue request", "Linked Queue", "O(1)", "all cases"},
        {"Push / pop undo record", "Linked Stack", "O(1)", "all cases"},
        {"Insert at list beginning", "Linked list", "O(1)", "all cases"},
        {"Insert at list end", "Linked list + tail", "O(1)", "all cases"},
        {"Insert at position k", "Linked list", "O(k) = O(n)", "worst"},
        {"Search resource by name", "Linear search", "O(n)", "worst (best O(1))"},
        {"Search sorted resources", "Binary search", "O(log n)", "worst (best O(1))"},
        {"Resource lookup by ID", "Hash table (chaining)", "O(1) avg, O(n) worst", "average"},
        {"Tree insert/search/delete", "AVL (balanced BST)", "O(log n)", "worst (plain BST: O(n))"},
        {"Insert / extract request", "Binary min-heap", "O(log n)", "worst"},
        {"Peek highest priority", "Binary min-heap", "O(1)", "all cases"},
        {"Sort resources", "Insertion sort", "O(n^2) avg, O(n) best", "average"},
        {"Sort resources", "Merge sort", "O(n log n)", "all cases, O(n) extra space"},
        {"Sort resources", "Quick sort", "O(n log n) avg, O(n^2) worst", "average"},
        {"Graph traversal", "BFS / DFS", "O(V + E)", "all cases"},
        {"Shortest path", "Dijkstra (array)", "O(V^2)", "all cases"},
    };
    for (const Row& row : rows) {
        cout << "  " << left << setw(28) << row.operation << setw(26) << row.structure
                  << setw(30) << row.complexity << row.caseType << "\n";
    }
}

void LibrarySystem::runBenchmark() const {
    cout << "\n  Generating random catalogues and timing the algorithms (microseconds)...\n"
              << "  Search columns show the TOTAL time for 1000 lookups.\n\n";
    cout << "  " << left << setw(8) << "n" << setw(12) << "Insertion" << setw(10) << "Merge"
              << setw(10) << "Quick" << setw(13) << "Linear x1000" << setw(13) << "Binary x1000"
              << setw(11) << "BST x1000" << "Hash x1000\n";
    Utils::printLine('-', 92);

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
        long long start = currentMicroseconds();
        Sorting::insertionSort(insertionData, byIdLess);
        long long insertionTime = currentMicroseconds() - start;
        start = currentMicroseconds();
        Sorting::mergeSort(mergeData, byIdLess);
        long long mergeTime = currentMicroseconds() - start;
        start = currentMicroseconds();
        Sorting::quickSort(quickData, byIdLess);
        long long quickTime = currentMicroseconds() - start;

        ResourceBST benchmarkTree;
        ResourceHashTable benchmarkTable(itemTotal / 2 + 1);   // load factor about 2
        for (int index = 0; index < itemTotal; ++index) {
            benchmarkTree.insert(dataset[index]);
            benchmarkTable.insert(dataset[index]);
        }

        // The query IDs are prepared first so that only the searching itself is timed.
        const int lookups = 1000;
        DynamicArray<string> queryIds;
        for (int query = 0; query < lookups; ++query) queryIds.pushBack(dataset[(query * 37) % itemTotal]->getId());

        // Every result is added to this "checksum". Without that, an optimising compiler
        // notices the results are never used and deletes the loops, which would make the
        // timings meaningless. 'volatile' forces the value to really be computed.
        volatile long long checksum = 0;
        int comparisons = 0;

        start = currentMicroseconds();
        for (int query = 0; query < lookups; ++query)
            checksum = checksum + Searching::linearSearchById(dataset, queryIds[query], comparisons);
        long long linearTime = currentMicroseconds() - start;

        start = currentMicroseconds();
        for (int query = 0; query < lookups; ++query)
            checksum = checksum + Searching::binarySearchById(mergeData, queryIds[query], comparisons);
        long long binaryTime = currentMicroseconds() - start;

        start = currentMicroseconds();
        for (int query = 0; query < lookups; ++query)
            checksum = checksum + (benchmarkTree.search(queryIds[query]) != nullptr ? 1 : 0);
        long long treeTime = currentMicroseconds() - start;

        start = currentMicroseconds();
        for (int query = 0; query < lookups; ++query)
            checksum = checksum + (benchmarkTable.find(queryIds[query]) != nullptr ? 1 : 0);
        long long hashTime = currentMicroseconds() - start;
        (void)checksum;

        cout << "  " << left << setw(8) << itemTotal << setw(12) << insertionTime
                  << setw(10) << mergeTime << setw(10) << quickTime << setw(13) << linearTime
                  << setw(13) << binaryTime << setw(11) << treeTime << hashTime << "\n";

        for (int index = 0; index < dataset.size(); ++index) delete dataset[index];
    }
    cout << "\n  Observation: insertion sort grows roughly with n^2 while merge/quick sort grow with n log n;\n"
                 "  linear search grows with n, while binary search, BST and hash lookups stay almost flat.\n";
}

void LibrarySystem::analysisMenu() const {
    bool stayInMenu = true;
    while (stayInMenu) {
        Utils::printHeader("10. SYSTEM ANALYSIS");
        cout << "  1. Complexity table of key operations\n"
                     "  2. Run the large-dataset benchmark (1000 / 5000 / 10000 items)\n"
                     "  3. Current structure sizes\n"
                     "  0. Back\n";
        int choice = Utils::readInt("Enter your choice: ", 0, 3);
        if (choice == 1) printComplexityTable();
        else if (choice == 2) runBenchmark();
        else if (choice == 3) {
            cout << "  Resources in linked list : " << resourceList.size() << "\n"
                      << "  Nodes in BST (height " << resourceTree.height() << ") : " << resourceTree.size() << "\n"
                      << "  Entries in hash table    : " << resourceHashTable.size() << "\n"
                      << "  Pending FIFO requests    : " << normalRequestQueue.size() << "\n"
                      << "  Pending priority requests: " << priorityRequestHeap.size() << "\n"
                      << "  Undo stack depth         : " << undoStack.size() << "\n"
                      << "  Campus locations         : " << campusGraph.locationCount() << "\n";
        } else stayInMenu = false;
    }
}

void LibrarySystem::loadSampleData() {
    auto addSample = [this](LibraryResource* resource) { insertResource(resource, resourceList.size()); };
    addSample(new Book("B001", "Introduction to Algorithms", "Main Library, Shelf A1", 5, AvailabilityStatus::AVAILABLE, "Cormen et al."));
    addSample(new Book("B002", "Data Structures in C++", "Main Library, Shelf A2", 4, AvailabilityStatus::AVAILABLE, "Mark Allen Weiss"));
    addSample(new Book("B003", "Clean Code", "Main Library, Shelf B1", 2, AvailabilityStatus::BORROWED, "Robert C. Martin"));
    addSample(new Book("B004", "Database System Concepts", "Main Library, Shelf B3", 3, AvailabilityStatus::AVAILABLE, "Silberschatz"));
    addSample(new Book("B005", "Operating System Concepts", "Main Library, Shelf C2", 3, AvailabilityStatus::AVAILABLE, "Silberschatz"));
    addSample(new Book("B006", "Discrete Mathematics", "Main Library, Shelf D4", 6, AvailabilityStatus::AVAILABLE, "Kenneth Rosen"));
    addSample(new Book("B007", "History of Lesotho", "Special Collection", 1, AvailabilityStatus::MAINTENANCE, "Moshoeshoe Archive"));
    addSample(new StudyRoom("R101", "Study Room 1", "Library Floor 1", 6, AvailabilityStatus::AVAILABLE, 1));
    addSample(new StudyRoom("R102", "Study Room 2", "Library Floor 1", 4, AvailabilityStatus::AVAILABLE, 1));
    addSample(new StudyRoom("R201", "Group Discussion Room", "Library Floor 2", 12, AvailabilityStatus::AVAILABLE, 2));
    addSample(new StudyRoom("R202", "Silent Study Hall", "Library Floor 2", 40, AvailabilityStatus::AVAILABLE, 2));
    addSample(new Equipment("E001", "Laptop Trolley", "Circulation Desk", 10, AvailabilityStatus::AVAILABLE, "Good"));
    addSample(new Equipment("E002", "Projector", "Circulation Desk", 3, AvailabilityStatus::AVAILABLE, "Good"));
    addSample(new Equipment("E003", "Book Scanner", "Digital Services", 2, AvailabilityStatus::MAINTENANCE, "Needs new lamp"));
    addSample(new Equipment("E004", "Portable Whiteboard", "Circulation Desk", 5, AvailabilityStatus::AVAILABLE, "Fair"));

    const char* names[] = {"Main Library", "Computer Science Building", "Administration Building", "Student Affairs",
                           "Academic Building", "Refectory", "Science Laboratories", "Sports Centre", "Staff Housing", "Campus Clinic"};
    for (const char* name : names) campusGraph.addLocation(name);
    struct Pathway { int from; int to; int meters; };
    const Pathway pathways[] = {{0, 1, 120}, {0, 4, 90}, {0, 5, 200}, {1, 4, 60}, {1, 6, 150}, {2, 3, 80},
                                {2, 4, 110}, {3, 5, 140}, {5, 7, 250}, {6, 7, 300}, {8, 9, 70}};
    for (const Pathway& pathway : pathways) campusGraph.addConnection(pathway.from, pathway.to, pathway.meters);

    auto addSampleRequest = [this](const string& user, const string& resourceId, PriorityLevel priority) {
        ResourceRequest request(nextRequestId++, user, resourceId, time(nullptr), RequestStatus::PENDING, priority);
        if (priority == PriorityLevel::NORMAL) normalRequestQueue.enqueue(request);
        else priorityRequestHeap.insert(request);
    };
    addSampleRequest("Thabo Mokoena", "B001", PriorityLevel::NORMAL);
    addSampleRequest("Palesa Khoase", "R101", PriorityLevel::NORMAL);
    addSampleRequest("Lerato Mohapi", "E002", PriorityLevel::NORMAL);
    addSampleRequest("Lecturer CS3400", "R201", PriorityLevel::HIGH);
    addSampleRequest("Head Librarian", "E003", PriorityLevel::EMERGENCY);
    addSampleRequest("Exam Office", "R202", PriorityLevel::HIGH);
}

bool LibrarySystem::loadResourcesFromFile() {
    ifstream input(resourceFilePath);
    if (!input.is_open()) return false;
    string line;
    while (getline(input, line)) {
        line = Utils::trim(line);
        if (line.empty()) continue;
        DynamicArray<string> fields = Utils::splitFields(line);
        int capacity = 0;
        if (fields.size() != 7 || !Utils::tryParseInt(fields[4], 0, 100000, capacity)) {
            cout << "  [load] skipped a malformed resource line.\n";
            continue;
        }
        LibraryResource* resource = ResourceFactory::create(fields[0], fields[1], fields[2], fields[3], capacity,
                                                            statusFromString(fields[5]), fields[6]);
        if (resource == nullptr || !insertResource(resource, resourceList.size())) {
            cout << "  [load] skipped invalid or duplicate resource line.\n";
            delete resource;
        }
    }
    return true;
}

void LibrarySystem::loadRequestsFromFile() {
    ifstream input(requestFilePath);
    if (!input.is_open()) return;
    string line;
    while (getline(input, line)) {
        line = Utils::trim(line);
        if (line.empty()) continue;
        DynamicArray<string> fields = Utils::splitFields(line);
        if (fields[0] == "NEXT" && fields.size() == 2) {
            Utils::tryParseInt(fields[1], 1, 100000000, nextRequestId);
            continue;
        }
        int requestId = 0, priorityNumber = 0;
        if (fields.size() != 7 || !Utils::tryParseInt(fields[1], 1, 100000000, requestId) ||
            !Utils::tryParseInt(fields[6], 1, 4, priorityNumber)) {
            cout << "  [load] skipped a malformed request line.\n";
            continue;
        }
        time_t submitted = static_cast<time_t>(Utils::parseLongLong(fields[4]));
        ResourceRequest request(requestId, fields[2], fields[3], submitted, requestStatusFromString(fields[5]),
                                static_cast<PriorityLevel>(priorityNumber));
        if (fields[0] == "N") normalRequestQueue.enqueue(request);
        else priorityRequestHeap.insert(request);
    }
}

void LibrarySystem::loadLocationsFromFile() {
    ifstream input(locationFilePath);
    if (!input.is_open()) return;
    string line;
    while (getline(input, line)) {
        line = Utils::trim(line);
        if (line.empty()) continue;
        DynamicArray<string> fields = Utils::splitFields(line);
        int meters = 0;
        if (fields[0] == "L" && fields.size() == 2) {
            campusGraph.addLocation(fields[1]);
        } else if (fields[0] == "E" && fields.size() == 4 && Utils::tryParseInt(fields[3], 1, 100000, meters)) {
            campusGraph.addConnection(campusGraph.findLocationIndex(fields[1]),
                                      campusGraph.findLocationIndex(fields[2]), meters);
        } else {
            cout << "  [load] skipped a malformed location line.\n";
        }
    }
}

LibrarySystem::~LibrarySystem() {
    // Free every object we own: the catalogue and the snapshots still held by the undo stack.
    resourceList.forEach([](LibraryResource* const& resource) { delete resource; });
    resourceList.clear();
    UndoRecord record;
    while (undoStack.pop(record)) delete record.resourceSnapshot;
}

void LibrarySystem::loadAllData() {
    if (loadResourcesFromFile()) {
        loadRequestsFromFile();
        loadLocationsFromFile();
        cout << "Loaded " << resourceList.size() << " resource(s), "
                  << (normalRequestQueue.size() + priorityRequestHeap.size()) << " pending request(s) and "
                  << campusGraph.locationCount() << " location(s) from the data files.\n";
    } else {
        loadSampleData();
        cout << "No saved data found - loaded sample data (" << resourceList.size() << " resources).\n";
    }
}

void LibrarySystem::saveAllData() const {
    ofstream resourceOutput(resourceFilePath);
    if (!resourceOutput.is_open()) {
        cout << "Warning: could not write the data files (is the current folder writable?).\n";
        return;
    }
    resourceList.forEach([&resourceOutput](LibraryResource* const& resource) {
        resourceOutput << resource->serialize() << "\n";
    });

    ofstream requestOutput(requestFilePath);
    requestOutput << "NEXT|" << nextRequestId << "\n";
    normalRequestQueue.forEach([&requestOutput](const ResourceRequest& request) {
        requestOutput << "N|" << request.serialize() << "\n";
    });
    DynamicArray<ResourceRequest> heapContents;
    priorityRequestHeap.copyItemsTo(heapContents);
    for (int index = 0; index < heapContents.size(); ++index) requestOutput << "P|" << heapContents[index].serialize() << "\n";

    ofstream locationOutput(locationFilePath);
    campusGraph.writeTo(locationOutput);
    cout << "Data saved to the data folder (resources.txt, requests.txt, locations.txt).\n";
}

void LibrarySystem::run() {
    bool running = true;
    while (running) {
        cout << "\n";
        Utils::printLine('=', 60);
        cout << "  CS3400 LIBRARY RESOURCE MANAGEMENT SYSTEM\n";
        Utils::printLine('=', 60);
        cout << "  1.  Resource Management\n"
                     "  2.  Search Resources\n"
                     "  3.  Sort Resources / Requests\n"
                     "  4.  Resource Requests (FIFO queue)\n"
                     "  5.  Priority Requests (heap)\n"
                     "  6.  Undo Operations (stack)\n"
                     "  7.  Tree Operations (BST)\n"
                     "  8.  Hash Table Lookup\n"
                     "  9.  Campus Navigation (graph)\n"
                     "  10. System Analysis\n"
                     "  0.  Exit (saves data)\n";
        switch (Utils::readInt("Enter your choice: ", 0, 10)) {
            case 1: resourceManagementMenu(); break;
            case 2: searchMenu(); break;
            case 3: sortMenu(); break;
            case 4: requestMenu(); break;
            case 5: priorityMenu(); break;
            case 6: undoMenu(); break;
            case 7: treeMenu(); break;
            case 8: hashMenu(); break;
            case 9: navigationMenu(); break;
            case 10: analysisMenu(); break;
            case 0: running = false; break;
        }
    }
}
