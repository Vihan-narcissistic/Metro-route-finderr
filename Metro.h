
#ifndef METRO_H
#define METRO_H

#include "Graph.h"
#include <unordered_map>
#include <vector>
#include <string>

struct InterchangeDetail {
    string station;
    string fromLine;
    string toLine;
};

struct RouteResult {
    bool success;
    string message;
    vector<string> path;
    int distance;
    int fare;
    int interchanges;
    vector<string> lineSegments; // Line taken between path[i] and path[i+1]
    vector<InterchangeDetail> interchangeList;
};

struct StationInfo {
    string name;
    vector<string> lines;
    bool isInterchange;
    vector<pair<string, int>> connections;
};

class Metro {
private:
    Graph metroGraph;

    unordered_map<string, vector<string>> stationLines;

    // Finds a line shared by two consecutive stations
    string findCommonLine(const string& station1,
                          const string& station2);

public:
    Metro();

    void addStation();
    void addConnection();
    void showStations();
    void showNetwork();
    void findShortestRoute();
    void performBFS();
    void performDFS();
    void calculateFare();

    void displayInterchanges();
    void displayStationLines();

    // New feature
    void findRouteWithInterchanges();

    // Programmatic APIs for backend/web integration
    RouteResult getRoute(const string& source, const string& destination);
    int calculateFareForDistance(int distance) const;
    vector<StationInfo> getAllStationDetails() const;
    vector<string> getBFSOrder(const string& start);
    vector<string> getDFSOrder(const string& start);
    bool addStationDirect(const string& name, const vector<string>& lines = {});
    bool addConnectionDirect(const string& s1, const string& s2, int distance);
    const unordered_map<string, vector<string>>& getStationLines() const;
    Graph& getGraph();
    const Graph& getGraph() const;
};

#endif
