
#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include <stack>
#include <limits>
#include <algorithm>

using namespace std;

class Graph {
private:
    // Station name -> Station ID
    unordered_map<string, int> stationMap;

    // Station ID -> Station name
    vector<string> stations;

    // Adjacency list: destination and distance
    vector<vector<pair<int, int>>> adj;

public:
    // Add a new metro station
    void addStation(const string& name);

    // Connect two stations
    void addConnection(const string& station1,
                       const string& station2,
                       int distance);

    // Display all metro stations
    void displayStations();

    // Display metro network
    void displayNetwork();

    // Find shortest route using Dijkstra
    void shortestRoute(const string& source,
                       const string& destination);
    int getShortestDistance(const string& source,
                        const string& destination);
    vector<string> getShortestPath(const string& source,
                        const string& destination);

    // Breadth First Search
    void BFS(const string& start);
    vector<string> getBFS(const string& start);

    // Depth First Search
    void DFS(const string& start);
    vector<string> getDFS(const string& start);

    // Check if station exists
    bool stationExists(const string& name);

    // Data getters for frontend/backend integration
    const vector<string>& getAllStations() const;
    const vector<vector<pair<int, int>>>& getAdjacencyList() const;
    vector<pair<string, int>> getNeighbors(const string& name) const;
};

#endif
