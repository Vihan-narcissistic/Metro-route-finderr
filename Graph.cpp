
#include "Graph.h"

bool Graph::stationExists(const string& name) {
    return stationMap.find(name) != stationMap.end();
}

void Graph::addStation(const string& name) {
    if (stationExists(name)) {
        cout << "Station already exists!\n";
        return;
    }

    int id = stations.size();

    stationMap[name] = id;
    stations.push_back(name);
    adj.push_back({});

    cout << "Station added successfully!\n";
}

void Graph::addConnection(const string& s1,
                          const string& s2,
                          int distance) {
    if (!stationExists(s1) || !stationExists(s2)) {
        cout << "One or both stations do not exist!\n";
        return;
    }

    if (distance <= 0) {
        cout << "Distance must be positive!\n";
        return;
    }

    int u = stationMap[s1];
    int v = stationMap[s2];

    adj[u].push_back({v, distance});
    adj[v].push_back({u, distance});

    cout << "Connection added successfully!\n";
}

void Graph::displayStations() {
    cout << "\n===== METRO STATIONS =====\n";

    for (int i = 0; i < stations.size(); i++) {
        cout << i + 1 << ". " << stations[i] << "\n";
    }
}

void Graph::displayNetwork() {
    cout << "\n===== METRO NETWORK =====\n";

    for (int i = 0; i < stations.size(); i++) {
        cout << stations[i] << " -> ";

        for (auto edge : adj[i]) {
            cout << stations[edge.first]
                 << " (" << edge.second << " km), ";
        }

        cout << "\n";
    }
}

void Graph::shortestRoute(const string& source,
                          const string& destination) {
    if (!stationExists(source) ||
        !stationExists(destination)) {
        cout << "Invalid station name!\n";
        return;
    }

    int src = stationMap[source];
    int dest = stationMap[destination];

    const int INF = numeric_limits<int>::max();

    vector<int> dist(stations.size(), INF);
    vector<int> parent(stations.size(), -1);

    // Min Heap: {distance, station ID}
    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        int currentDist = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (currentDist != dist[u])
            continue;

        for (auto edge : adj[u]) {
            int v = edge.first;
            int weight = edge.second;

            if (dist[u] <= INF - weight &&
                dist[u] + weight < dist[v]) {

                dist[v] = dist[u] + weight;
                parent[v] = u;

                pq.push({dist[v], v});
            }
        }
    }

    if (dist[dest] == INF) {
        cout << "No route available!\n";
        return;
    }

    vector<int> path;

    for (int v = dest; v != -1; v = parent[v]) {
        path.push_back(v);
    }

    reverse(path.begin(), path.end());

    cout << "\n===== SHORTEST ROUTE =====\n";

    for (int i = 0; i < path.size(); i++) {
        cout << stations[path[i]];

        if (i != path.size() - 1)
            cout << " -> ";
    }

    cout << "\nTotal Distance: "
         << dist[dest] << " km\n";
}

void Graph::BFS(const string& start) {
    if (!stationExists(start)) {
        cout << "Invalid station!\n";
        return;
    }

    vector<bool> visited(stations.size(), false);
    queue<int> q;

    int src = stationMap[start];

    visited[src] = true;
    q.push(src);

    cout << "\nBFS Traversal: ";

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        cout << stations[u] << " ";

        for (auto edge : adj[u]) {
            int v = edge.first;

            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }

    cout << "\n";
}

void Graph::DFS(const string& start) {
    if (!stationExists(start)) {
        cout << "Invalid station!\n";
        return;
    }

    vector<bool> visited(stations.size(), false);
    stack<int> st;

    int src = stationMap[start];

    st.push(src);

    cout << "\nDFS Traversal: ";

    while (!st.empty()) {
        int u = st.top();
        st.pop();

        if (visited[u])
            continue;

        visited[u] = true;
        cout << stations[u] << " ";

        for (auto it = adj[u].rbegin();
             it != adj[u].rend(); ++it) {

            if (!visited[it->first]) {
                st.push(it->first);
            }
        }
    }

    cout << "\n";
}

int Graph::getShortestDistance(const string& source,
                               const string& destination) {

    if (!stationExists(source) ||
        !stationExists(destination)) {
        return -1;
    }

    int src = stationMap[source];
    int dest = stationMap[destination];

    const int INF = numeric_limits<int>::max();

    vector<int> dist(stations.size(), INF);

    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {

        int currentDist = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (currentDist != dist[u])
            continue;

        if (u == dest)
            return dist[u];

        for (auto edge : adj[u]) {

            int v = edge.first;
            int weight = edge.second;

            if (dist[u] <= INF - weight &&
                dist[u] + weight < dist[v]) {

                dist[v] = dist[u] + weight;
                pq.push({dist[v], v});
            }
        }
    }

    return -1;
}

vector<string> Graph::getShortestPath(const string& source,
                                      const string& destination) {

    if (!stationExists(source) || !stationExists(destination)) {
        return {};
    }

    int src = stationMap[source];
    int dest = stationMap[destination];

    const int INF = numeric_limits<int>::max();

    vector<int> dist(stations.size(), INF);
    vector<int> parent(stations.size(), -1);

    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {

        int currentDist = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (currentDist != dist[u])
            continue;

        if (u == dest)
            break;

        for (auto edge : adj[u]) {

            int v = edge.first;
            int weight = edge.second;

            if (dist[u] <= INF - weight &&
                dist[u] + weight < dist[v]) {

                dist[v] = dist[u] + weight;
                parent[v] = u;

                pq.push({dist[v], v});
            }
        }
    }

    if (dist[dest] == INF) {
        return {};
    }

    vector<string> path;

    for (int v = dest; v != -1; v = parent[v]) {
        path.push_back(stations[v]);
    }

    reverse(path.begin(), path.end());

    return path;
}

vector<string> Graph::getBFS(const string& start) {
    if (!stationExists(start)) {
        return {};
    }

    vector<string> result;
    vector<bool> visited(stations.size(), false);
    queue<int> q;

    int src = stationMap[start];
    visited[src] = true;
    q.push(src);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        result.push_back(stations[u]);

        for (const auto& edge : adj[u]) {
            int v = edge.first;
            if (!visited[v]) {
                visited[v] = true;
                q.push(v);
            }
        }
    }

    return result;
}

vector<string> Graph::getDFS(const string& start) {
    if (!stationExists(start)) {
        return {};
    }

    vector<string> result;
    vector<bool> visited(stations.size(), false);
    stack<int> st;

    int src = stationMap[start];
    st.push(src);

    while (!st.empty()) {
        int u = st.top();
        st.pop();

        if (visited[u])
            continue;

        visited[u] = true;
        result.push_back(stations[u]);

        for (auto it = adj[u].rbegin(); it != adj[u].rend(); ++it) {
            if (!visited[it->first]) {
                st.push(it->first);
            }
        }
    }

    return result;
}

const vector<string>& Graph::getAllStations() const {
    return stations;
}

const vector<vector<pair<int, int>>>& Graph::getAdjacencyList() const {
    return adj;
}

vector<pair<string, int>> Graph::getNeighbors(const string& name) const {
    auto it = stationMap.find(name);
    if (it == stationMap.end()) return {};

    int u = it->second;
    vector<pair<string, int>> neighbors;
    for (const auto& edge : adj[u]) {
        neighbors.push_back({stations[edge.first], edge.second});
    }
    return neighbors;
}


