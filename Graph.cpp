#include "Graph.h"
#include <iostream>
#include <iomanip>
#include "Queue.h"
#include "Utils.h"

void CampusGraph::appendEdge(int fromIndex, int toIndex, int meters) {
    Edge* newEdge = new Edge(toIndex, meters);
    if (adjacencyLists[fromIndex] == nullptr) {
        adjacencyLists[fromIndex] = newEdge;
    } else {
        Edge* current = adjacencyLists[fromIndex];
        while (current->next != nullptr) current = current->next;
        current->next = newEdge;
    }
}

void CampusGraph::depthFirstVisit(int current, bool* visited, DynamicArray<int>& visitOrder) const {
    visited[current] = true;
    visitOrder.pushBack(current);
    for (Edge* edge = adjacencyLists[current]; edge != nullptr; edge = edge->next) {
        if (!visited[edge->destination]) depthFirstVisit(edge->destination, visited, visitOrder);
    }
}

CampusGraph::~CampusGraph() {
    for (int index = 0; index < adjacencyLists.size(); ++index) {
        Edge* current = adjacencyLists[index];
        while (current != nullptr) {
            Edge* next = current->next;
            delete current;
            current = next;
        }
    }
}

int CampusGraph::findLocationIndex(const string& name) const {
    string wanted = Utils::toLower(name);
    for (int index = 0; index < locationNames.size(); ++index) {
        if (Utils::toLower(locationNames[index]) == wanted) return index;
    }
    return -1;
}

bool CampusGraph::addLocation(const string& name) {
    if (findLocationIndex(name) != -1) return false;
    locationNames.pushBack(name);
    adjacencyLists.pushBack(nullptr);
    return true;
}

bool CampusGraph::hasConnection(int firstIndex, int secondIndex) const {
    for (Edge* edge = adjacencyLists[firstIndex]; edge != nullptr; edge = edge->next) {
        if (edge->destination == secondIndex) return true;
    }
    return false;
}

bool CampusGraph::addConnection(int firstIndex, int secondIndex, int meters) {
    if (firstIndex < 0 || secondIndex < 0 || firstIndex >= locationCount() || secondIndex >= locationCount()) return false;
    if (firstIndex == secondIndex || hasConnection(firstIndex, secondIndex)) return false;
    appendEdge(firstIndex, secondIndex, meters);
    appendEdge(secondIndex, firstIndex, meters);
    return true;
}

void CampusGraph::display() const {
    if (locationNames.isEmpty()) {
        cout << "  The campus graph is empty.\n";
        return;
    }
    for (int index = 0; index < locationNames.size(); ++index) {
        cout << "  " << left << setw(28) << locationNames[index] << " -> ";
        if (adjacencyLists[index] == nullptr) cout << "(no pathways)";
        for (Edge* edge = adjacencyLists[index]; edge != nullptr; edge = edge->next) {
            cout << locationNames[edge->destination] << " (" << edge->distanceMeters << "m)"
                      << (edge->next ? ", " : "");
        }
        cout << "\n";
    }
}

void CampusGraph::breadthFirstOrder(int startIndex, DynamicArray<int>& visitOrder) const {
    int total = locationCount();
    bool* visited = new bool[total]();
    Queue<int> toVisit;
    visited[startIndex] = true;
    toVisit.enqueue(startIndex);
    int current = 0;
    while (toVisit.dequeue(current)) {
        visitOrder.pushBack(current);
        for (Edge* edge = adjacencyLists[current]; edge != nullptr; edge = edge->next) {
            if (!visited[edge->destination]) {
                visited[edge->destination] = true;
                toVisit.enqueue(edge->destination);
            }
        }
    }
    delete[] visited;
}

void CampusGraph::depthFirstOrder(int startIndex, DynamicArray<int>& visitOrder) const {
    bool* visited = new bool[locationCount()]();
    depthFirstVisit(startIndex, visited, visitOrder);
    delete[] visited;
}

bool CampusGraph::isReachable(int fromIndex, int toIndex) const {
    DynamicArray<int> visitOrder;
    breadthFirstOrder(fromIndex, visitOrder);
    for (int index = 0; index < visitOrder.size(); ++index) {
        if (visitOrder[index] == toIndex) return true;
    }
    return false;
}

int CampusGraph::shortestPath(int sourceIndex, int targetIndex, DynamicArray<int>& pathOutput) const {
    int total = locationCount();
    int*  distance = new int[total];
    int*  previous = new int[total];
    bool* finalised = new bool[total]();
    for (int index = 0; index < total; ++index) {
        distance[index] = INFINITE_DISTANCE;
        previous[index] = -1;
    }
    distance[sourceIndex] = 0;

    for (int round = 0; round < total; ++round) {
        int closest = -1;
        for (int index = 0; index < total; ++index) {
            if (!finalised[index] && distance[index] != INFINITE_DISTANCE &&
                (closest == -1 || distance[index] < distance[closest])) {
                closest = index;
            }
        }
        if (closest == -1) break;
        finalised[closest] = true;
        for (Edge* edge = adjacencyLists[closest]; edge != nullptr; edge = edge->next) {
            int candidate = distance[closest] + edge->distanceMeters;
            if (!finalised[edge->destination] && candidate < distance[edge->destination]) {
                distance[edge->destination] = candidate;
                previous[edge->destination] = closest;
            }
        }
    }

    int result = (distance[targetIndex] == INFINITE_DISTANCE) ? -1 : distance[targetIndex];
    if (result != -1) {
        DynamicArray<int> reversedPath;
        for (int at = targetIndex; at != -1; at = previous[at]) reversedPath.pushBack(at);
        for (int index = reversedPath.size() - 1; index >= 0; --index) pathOutput.pushBack(reversedPath[index]);
    }
    delete[] distance;
    delete[] previous;
    delete[] finalised;
    return result;
}

int CampusGraph::printConnectedComponents() const {
    int total = locationCount();
    bool* visited = new bool[total]();
    int componentNumber = 0;
    for (int start = 0; start < total; ++start) {
        if (visited[start]) continue;
        DynamicArray<int> members;
        depthFirstVisit(start, visited, members);
        ++componentNumber;
        cout << "  Component " << componentNumber << ": ";
        for (int index = 0; index < members.size(); ++index) {
            cout << locationNames[members[index]] << (index + 1 < members.size() ? ", " : "");
        }
        cout << "\n";
    }
    delete[] visited;
    return componentNumber;
}

void CampusGraph::writeTo(ostream& output) const {
    for (int index = 0; index < locationNames.size(); ++index) output << "L|" << locationNames[index] << "\n";
    for (int index = 0; index < locationNames.size(); ++index) {
        for (Edge* edge = adjacencyLists[index]; edge != nullptr; edge = edge->next) {
            if (index < edge->destination) {   // write each undirected edge once
                output << "E|" << locationNames[index] << "|" << locationNames[edge->destination] << "|"
                       << edge->distanceMeters << "\n";
            }
        }
    }
}
