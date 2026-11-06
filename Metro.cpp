
#include "Metro.h"

Metro::Metro() {

    // Sample metro stations
    metroGraph.addStation("Rajiv Chowk");
    metroGraph.addStation("Kashmere Gate");
    metroGraph.addStation("New Delhi");
    metroGraph.addStation("Central Secretariat");
    metroGraph.addStation("Chandni Chowk");
    metroGraph.addStation("Mandi House");
    metroGraph.addStation("Patel Chowk");
    metroGraph.addStation("Karol Bagh");

    // Connections with approximate sample distances (km)
    metroGraph.addConnection("Rajiv Chowk", "New Delhi", 2);
    metroGraph.addConnection("Rajiv Chowk", "Mandi House", 2);
    metroGraph.addConnection("Rajiv Chowk", "Karol Bagh", 3);
    metroGraph.addConnection("New Delhi", "Chandni Chowk", 2);
    metroGraph.addConnection("Chandni Chowk", "Kashmere Gate", 3);
    metroGraph.addConnection("Mandi House", "Central Secretariat", 3);
    metroGraph.addConnection("Central Secretariat", "Patel Chowk", 2);
    metroGraph.addConnection("Patel Chowk", "Rajiv Chowk", 2);

    
    // Assign metro lines to stations

    stationLines["Rajiv Chowk"] =
        {"Blue Line", "Yellow Line"};

    stationLines["Karol Bagh"] =
        {"Blue Line"};

    stationLines["Mandi House"] =
        {"Blue Line", "Violet Line"};

    stationLines["New Delhi"] =
        {"Yellow Line"};

    stationLines["Chandni Chowk"] =
        {"Yellow Line"};

    stationLines["Kashmere Gate"] =
        {"Yellow Line", "Red Line"};

    stationLines["Central Secretariat"] =
        {"Yellow Line", "Violet Line"};

    stationLines["Patel Chowk"] =
        {"Yellow Line"};

}

void Metro::addStation() {
    string name;

    cout << "Enter station name: ";
    getline(cin >> ws, name);

    metroGraph.addStation(name);
}

void Metro::addConnection() {
    string s1, s2;
    int distance;

    cout << "Enter first station: ";
    getline(cin >> ws, s1);

    cout << "Enter second station: ";
    getline(cin >> ws, s2);

    cout << "Enter distance in km: ";
    if (!(cin >> distance)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid distance input!\n";
        return;
    }

    metroGraph.addConnection(s1, s2, distance);
}

void Metro::showStations() {
    metroGraph.displayStations();
}

void Metro::showNetwork() {
    metroGraph.displayNetwork();
}

void Metro::findShortestRoute() {
    string source, destination;

    cout << "Enter starting station: ";
    getline(cin >> ws, source);

    cout << "Enter destination station: ";
    getline(cin >> ws, destination);

    metroGraph.shortestRoute(source, destination);
}

void Metro::performBFS() {
    string start;

    cout << "Enter starting station: ";
    getline(cin >> ws, start);

    metroGraph.BFS(start);
}

void Metro::performDFS() {
    string start;

    cout << "Enter starting station: ";
    getline(cin >> ws, start);

    metroGraph.DFS(start);
}


void Metro::calculateFare() {

    string source, destination;

    cout << "\n===== METRO FARE CALCULATOR =====\n";

    cout << "Enter starting station: ";
    getline(cin >> ws, source);

    cout << "Enter destination station: ";
    getline(cin >> ws, destination);

    int distance =
        metroGraph.getShortestDistance(source, destination);

    if (distance == -1) {
        cout << "\nInvalid station or no route available!\n";
        return;
    }

    int fare = 0;

    if (distance == 0) {
        fare = 0;
    }
    else if (distance <= 2) {
        fare = 10;
    }
    else {
        fare = 10 + ((distance - 2 + 1) / 2) * 5;
    }

    cout << "\n========== FARE DETAILS ==========\n";

    cout << "Source Station      : " << source << "\n";
    cout << "Destination Station : " << destination << "\n";
    cout << "Shortest Distance   : " << distance << " km\n";
    cout << "Total Fare          : Rs. " << fare << "\n";

    cout << "==================================\n";
}
 
void Metro::displayInterchanges() {

    cout << "\n===== METRO INTERCHANGE STATIONS =====\n";

    bool found = false;

    for (const auto& station : stationLines) {

        if (station.second.size() > 1) {

            found = true;

            cout << "\nStation: " << station.first << "\n";

            cout << "Connected Lines: ";

            for (int i = 0;
                 i < station.second.size(); i++) {

                cout << station.second[i];

                if (i != station.second.size() - 1)
                    cout << ", ";
            }

            cout << "\nNumber of Lines: "
                 << station.second.size() << "\n";
        }
    }

    if (!found) {
        cout << "No interchange stations found.\n";
    }

    cout << "======================================\n";
}
 
void Metro::displayStationLines() {

    string name;

    cout << "\nEnter station name: ";
    getline(cin >> ws, name);

    auto it = stationLines.find(name);

    if (it == stationLines.end()) {

        cout << "No line information available for this station.\n";
        return;
    }

    cout << "\nStation: " << name << "\n";
    cout << "Metro Lines:\n";

    for (const string& line : it->second) {
        cout << "- " << line << "\n";
    }

    if (it->second.size() > 1) {
        cout << "This is an INTERCHANGE station.\n";
    }
    else {
        cout << "This is a regular station in the sample dataset.\n";
    }
}
void Metro::findRouteWithInterchanges() {

    string source, destination;

    cout << "\n===== ROUTE AND INTERCHANGE FINDER =====\n";

    cout << "Enter starting station: ";
    getline(cin >> ws, source);

    cout << "Enter destination station: ";
    getline(cin >> ws, destination);

    vector<string> path =
        metroGraph.getShortestPath(source, destination);

    if (path.empty()) {
        cout << "Invalid station or no route available!\n";
        return;
    }

    int distance =
        metroGraph.getShortestDistance(source, destination);

    cout << "\nShortest Route:\n";

    for (int i = 0; i < path.size(); i++) {
        cout << path[i];

        if (i != path.size() - 1)
            cout << " -> ";
    }

    cout << "\nTotal Distance: " << distance << " km\n";

    if (path.size() == 1) {
        cout << "Interchanges: 0\n";
        cout << "You are already at your destination.\n";
        return;
    }

    // Find available lines for every route segment.
    vector<vector<string>> edgeLines;

    for (int i = 0; i < path.size() - 1; i++) {

        vector<string> commonLines;

        auto first = stationLines.find(path[i]);
        auto second = stationLines.find(path[i + 1]);

        if (first != stationLines.end() &&
            second != stationLines.end()) {

            for (const string& line : first->second) {

                if (find(second->second.begin(),
                         second->second.end(),
                         line) != second->second.end()) {

                    commonLines.push_back(line);
                }
            }
        }

        if (commonLines.empty()) {
            cout << "\nLine information is incomplete for this route.\n";
            cout << "Interchange count cannot be calculated reliably.\n";
            return;
        }

        edgeLines.push_back(commonLines);
    }

    // Dynamic programming state.
    struct State {
        int changes;
        string previousLine;
    };

    vector<unordered_map<string, State>> dp(edgeLines.size());

    // First route segment: no line change yet.
    for (const string& line : edgeLines[0]) {
        dp[0][line] = {0, ""};
    }

    // Minimize line changes for subsequent segments.
    for (int i = 1; i < edgeLines.size(); i++) {

        for (const string& currentLine : edgeLines[i]) {

            int bestChanges = numeric_limits<int>::max();
            string bestPreviousLine;

            for (const auto& previous : dp[i - 1]) {

                int changes = previous.second.changes;

                if (previous.first != currentLine) {
                    changes++;
                }

                if (changes < bestChanges) {
                    bestChanges = changes;
                    bestPreviousLine = previous.first;
                }
            }

            dp[i][currentLine] =
                {bestChanges, bestPreviousLine};
        }
    }

    // Find the best final line.
    int minimumChanges = numeric_limits<int>::max();
    string currentLine;

    for (const auto& entry : dp.back()) {

        if (entry.second.changes < minimumChanges) {
            minimumChanges = entry.second.changes;
            currentLine = entry.first;
        }
    }

    // Reconstruct the selected line sequence.
    vector<string> selectedLines(edgeLines.size());

    for (int i = static_cast<int>(edgeLines.size()) - 1;
         i >= 0; i--) {

        selectedLines[i] = currentLine;

        currentLine = dp[i][currentLine].previousLine;
    }

    // Display route with selected metro lines.
    cout << "\nRoute Line Details:\n";

    for (int i = 0; i < selectedLines.size(); i++) {

        cout << path[i] << " -> "
             << path[i + 1]
             << " : " << selectedLines[i] << "\n";
    }

    // Display actual line-change stations.
    cout << "\nInterchange Stations:\n";

    if (minimumChanges == 0) {
        cout << "No interchange required.\n";
    }
    else {

        int count = 0;

        for (int i = 1; i < selectedLines.size(); i++) {

            if (selectedLines[i] != selectedLines[i - 1]) {

                count++;

                cout << count << ". " << path[i]
                     << " (" << selectedLines[i - 1]
                     << " -> " << selectedLines[i] << ")\n";
            }
        }
    }

    cout << "\nTotal Interchanges: "
         << minimumChanges << "\n";

    cout << "========================================\n";
}

int Metro::calculateFareForDistance(int distance) const {
    if (distance <= 0) {
        return 0;
    }
    else if (distance <= 2) {
        return 10;
    }
    else {
        return 10 + ((distance - 2 + 1) / 2) * 5;
    }
}

RouteResult Metro::getRoute(const string& source, const string& destination) {
    RouteResult result;
    result.success = false;
    result.distance = -1;
    result.fare = 0;
    result.interchanges = 0;

    if (!metroGraph.stationExists(source)) {
        result.message = "Starting station '" + source + "' does not exist.";
        return result;
    }
    if (!metroGraph.stationExists(destination)) {
        result.message = "Destination station '" + destination + "' does not exist.";
        return result;
    }

    if (source == destination) {
        result.success = true;
        result.message = "Source and destination are identical.";
        result.path = {source};
        result.distance = 0;
        result.fare = 0;
        result.interchanges = 0;
        return result;
    }

    vector<string> path = metroGraph.getShortestPath(source, destination);
    if (path.empty()) {
        result.message = "No route found between " + source + " and " + destination + ".";
        return result;
    }

    int distance = metroGraph.getShortestDistance(source, destination);
    result.success = true;
    result.message = "Shortest route computed successfully.";
    result.path = path;
    result.distance = distance;
    result.fare = calculateFareForDistance(distance);

    // Identify common lines for each segment
    vector<vector<string>> edgeLines;
    for (size_t i = 0; i < path.size() - 1; i++) {
        vector<string> common;
        auto first = stationLines.find(path[i]);
        auto second = stationLines.find(path[i + 1]);

        if (first != stationLines.end() && second != stationLines.end()) {
            for (const string& line : first->second) {
                if (find(second->second.begin(), second->second.end(), line) != second->second.end()) {
                    common.push_back(line);
                }
            }
        }

        if (common.empty()) {
            common.push_back("Standard Line");
        }
        edgeLines.push_back(common);
    }

    // Dynamic Programming to minimize line changes
    struct State {
        int changes;
        string previousLine;
    };

    vector<unordered_map<string, State>> dp(edgeLines.size());

    // First segment
    for (const string& line : edgeLines[0]) {
        dp[0][line] = {0, ""};
    }

    // Subsequent segments
    for (size_t i = 1; i < edgeLines.size(); i++) {
        for (const string& currentLine : edgeLines[i]) {
            int bestChanges = numeric_limits<int>::max();
            string bestPreviousLine;

            for (const auto& previous : dp[i - 1]) {
                int changes = previous.second.changes;
                if (previous.first != currentLine) {
                    changes++;
                }
                if (changes < bestChanges) {
                    bestChanges = changes;
                    bestPreviousLine = previous.first;
                }
            }
            dp[i][currentLine] = {bestChanges, bestPreviousLine};
        }
    }

    // Find best ending line
    int minChanges = numeric_limits<int>::max();
    string currentLine;
    for (const auto& entry : dp.back()) {
        if (entry.second.changes < minChanges) {
            minChanges = entry.second.changes;
            currentLine = entry.first;
        }
    }

    // Backtrack selected lines
    vector<string> selectedLines(edgeLines.size());
    for (int i = static_cast<int>(edgeLines.size()) - 1; i >= 0; i--) {
        selectedLines[i] = currentLine;
        currentLine = dp[i][currentLine].previousLine;
    }

    result.lineSegments = selectedLines;
    result.interchanges = minChanges;

    // Identify interchange stations
    for (size_t i = 1; i < selectedLines.size(); i++) {
        if (selectedLines[i] != selectedLines[i - 1]) {
            InterchangeDetail detail;
            detail.station = path[i];
            detail.fromLine = selectedLines[i - 1];
            detail.toLine = selectedLines[i];
            result.interchangeList.push_back(detail);
        }
    }

    return result;
}

vector<StationInfo> Metro::getAllStationDetails() const {
    vector<StationInfo> list;
    const vector<string>& all = metroGraph.getAllStations();
    for (const string& name : all) {
        StationInfo info;
        info.name = name;
        auto it = stationLines.find(name);
        if (it != stationLines.end()) {
            info.lines = it->second;
        } else {
            info.lines = {"Unassigned Line"};
        }
        info.isInterchange = (info.lines.size() > 1);
        info.connections = metroGraph.getNeighbors(name);
        list.push_back(info);
    }
    return list;
}

vector<string> Metro::getBFSOrder(const string& start) {
    return metroGraph.getBFS(start);
}

vector<string> Metro::getDFSOrder(const string& start) {
    return metroGraph.getDFS(start);
}

bool Metro::addStationDirect(const string& name, const vector<string>& lines) {
    if (name.empty() || metroGraph.stationExists(name)) {
        return false;
    }
    metroGraph.addStation(name);
    if (!lines.empty()) {
        stationLines[name] = lines;
    } else {
        stationLines[name] = {"Direct Line"};
    }
    return true;
}

bool Metro::addConnectionDirect(const string& s1, const string& s2, int distance) {
    if (!metroGraph.stationExists(s1) || !metroGraph.stationExists(s2) || distance <= 0) {
        return false;
    }
    metroGraph.addConnection(s1, s2, distance);
    return true;
}

const unordered_map<string, vector<string>>& Metro::getStationLines() const {
    return stationLines;
}

Graph& Metro::getGraph() {
    return metroGraph;
}

const Graph& Metro::getGraph() const {
    return metroGraph;
}

