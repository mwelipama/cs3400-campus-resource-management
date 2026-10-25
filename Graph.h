#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <string>
#include "DynamicArray.h"

using namespace std;

// Weighted undirected graph using adjacency lists. BFS and DFS are O(V + E); Dijkstra (array version) is O(V^2).
class CampusGraph {
private:
    struct Edge {
        int   destination;
        int   distanceMeters;
        Edge* next;
        Edge(int destinationIndex, int meters) : destination(destinationIndex), distanceMeters(meters), next(nullptr) {}
    };

    static constexpr int INFINITE_DISTANCE = 2147483647;   // "not reached yet"

    DynamicArray<string> locationNames;
    DynamicArray<Edge*>       adjacencyLists;

    void appendEdge(int fromIndex, int toIndex, int meters);

    void depthFirstVisit(int current, bool* visited, DynamicArray<int>& visitOrder) const;

public:
    CampusGraph() {}
    CampusGraph(const CampusGraph&) = delete;
    CampusGraph& operator=(const CampusGraph&) = delete;

    ~CampusGraph();

    int locationCount() const { return locationNames.size(); }
    const string& locationName(int index) const { return locationNames[index]; }

    int findLocationIndex(const string& name) const;

    bool addLocation(const string& name);

    bool hasConnection(int firstIndex, int secondIndex) const;

    // Undirected pathway: stored in both directions.
    bool addConnection(int firstIndex, int secondIndex, int meters);

    void display() const;

    void breadthFirstOrder(int startIndex, DynamicArray<int>& visitOrder) const;

    void depthFirstOrder(int startIndex, DynamicArray<int>& visitOrder) const;

    bool isReachable(int fromIndex, int toIndex) const;

    // Dijkstra's algorithm. Returns the total distance in metres or -1 if unreachable.
    int shortestPath(int sourceIndex, int targetIndex, DynamicArray<int>& pathOutput) const;

    int printConnectedComponents() const;

    void writeTo(ostream& output) const;
};
#endif
