# Metro Route Finder – C++ DSA Web & CLI Application

An interactive, high-performance metro route planning application built using fundamental **Data Structures and Algorithms (DSA) in C++**, featuring both an interactive **modern web dashboard** and the original **console CLI application**.

---

## 1. System Architecture

All graph routing, graph traversals, fare calculations, and interchange optimization algorithms execute strictly in **native C++**. The web frontend acts as a responsive visualization layer communicating with a lightweight **C++ Winsock2 HTTP Server**.

```text
┌────────────────────────────────────────────────────────────────────────┐
│               Modern Web Frontend (HTML5 / CSS3 / Vanilla JS)          │
│   - Route Planner (Station Selectors, Journey Timeline, Fare Display)  │
│   - DSA Traversal Visualizer (BFS & DFS Animated Steps)                │
│   - Interactive Metro Network Graph (SVG Tracks & Nodes)               │
│   - Live Station & Track Connection Management                         │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ HTTP REST (JSON)
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│           Cross-Platform C++ HTTP Server (server.cpp)                  │
│   - Windows: Native Winsock2 (winsock2.h / -lws2_32)                   │
│   - Linux / Docker: Native POSIX Sockets (sys/socket.h, unistd.h)      │
│   - Reads PORT environment variable (defaults to 8080)                 │
│   - Bound to 0.0.0.0 (enables Docker, Render & Cloud hosting)          │
│   - Zero external 3rd-party dependencies                               │
│   - Serves static frontend files (HTML/CSS/JS) & REST API endpoints    │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   Metro Layer (Metro.h / Metro.cpp)                    │
│   - Fare Calculation Engine (Tiered distance pricing)                  │
│   - Dynamic Programming Line Minimization Algorithm                    │
│   - Station metadata & Metro line memberships                          │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                  Graph DSA Engine (Graph.h / Graph.cpp)                │
│   - Adjacency List: vector<vector<pair<int, int>>> adj                 │
│   - Dijkstra's Algorithm using std::priority_queue (Min-Heap)          │
│   - Breadth-First Search (BFS) using std::queue (FIFO)                 │
│   - Depth-First Search (DFS) using std::stack (LIFO)                   │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Data Structures & Algorithms

| Feature | Data Structure / Algorithm | Time Complexity | Role in Project |
|---|---|---|---|
| **Station Mapping** | `std::unordered_map<string, int>` | $O(1)$ average | Maps station names to graph node indices |
| **Network Representation** | Adjacency List `vector<vector<pair<int, int>>>` | $O(V + E)$ space | Stores undirected weighted connections between stations |
| **Shortest Route** | **Dijkstra's Algorithm** with `std::priority_queue` | $O((V + E) \log V)$ | Computes the absolute minimum distance path between stations |
| **Line Transfer Optimization**| **Dynamic Programming (DP)** | $O(S \times L^2)$ | Keeps shortest path fixed while choosing metro lines that minimize transfers |
| **BFS Traversal** | **Breadth-First Search** with `std::queue` | $O(V + E)$ | Traverses the graph level-by-level (FIFO) |
| **DFS Traversal** | **Depth-First Search** with `std::stack` | $O(V + E)$ | Traverses deep down branches before backtracking (LIFO) |
| **Fare Estimation** | Constant-time policy calculation | $O(1)$ | Tiered formula based on travel distance |

### Fare Calculation Formula
$$\text{Fare} = \begin{cases} 
0 & \text{if distance} = 0\text{ km} \\
10 & \text{if distance} \le 2\text{ km} \\
10 + \lceil\frac{\text{distance} - 2}{2}\rceil \times 5 & \text{if distance} > 2\text{ km}
\end{cases}$$

---

## 3. Project Directory Structure

```text
METRO-APP-CPP/
│
├── frontend/                     # Modern Web UI (Pure Vanilla Web)
│   ├── index.html                # Semantic HTML5 transit dashboard
│   ├── style.css                 # Responsive stylesheet & metro line themes
│   └── script.js                 # API communication & interactive SVG map
│
├── Dockerfile                    # Linux multi-stage Docker build for cloud/Render
├── .dockerignore                 # Excludes binaries and local artifacts from image
├── main.cpp                      # Classic Console CLI application
├── server.cpp                    # Cross-platform (Windows & Linux) C++ HTTP Server
│
├── Graph.h                       # Graph class header (Dijkstra, BFS, DFS)
├── Graph.cpp                     # Graph algorithm implementations
├── Metro.h                       # Metro class header (Lines, Fares, Interchanges)
├── Metro.cpp                     # Metro business logic & DP line optimization
└── README.md                     # Project documentation
```

---

## 4. How to Compile and Run

The project supports **Windows local execution**, **Linux native compilation**, **Docker containerization**, and **Cloud deployment (Render / Railway / Fly.io)**.

---

### Option A: Run Modern Web Application Locally on Windows (MinGW)

#### Step 1: Open PowerShell
```powershell
cd C:\Users\VIHAN\Documents\METRO-APP-CPP
```

#### Step 2: Compile the C++ HTTP Backend
```powershell
g++ -std=c++14 server.cpp Graph.cpp Metro.cpp -o metro_server.exe -lws2_32
```

#### Step 3: Start the Backend Server
```powershell
.\metro_server.exe
```

#### Step 4: Open in Web Browser
Open your browser and navigate to:
```text
(https://metro-route-finderr.onrender.com)
```


### Option B: Build and Run with Docker (Linux / Cloud / Render)

#### Step 1: Build the Docker Image
```bash
docker build -t metro-route-finder .
```

#### Step 2: Run the Docker Container
```bash
docker run -d -p 8080:8080 -e PORT=8080 --name metro-app metro-route-finder
```

#### Step 3: Access the Web App
Open your browser at `https://metro-route-finderr.onrender.com`.

---

### Option C: Compile on Native Linux / macOS (g++ POSIX)

On any Linux machine (Ubuntu, Debian, Fedora, Arch):
```bash
# Compile (no -lws2_32 needed on Linux)
g++ -std=c++14 -O2 server.cpp Graph.cpp Metro.cpp -o metro_server

# Run
(https://metro-route-finderr.onrender.com)
```

---

### Option D: Run Classic Console CLI Application

#### Step 1: Compile the CLI
```powershell
g++ -std=c++14 main.cpp Graph.cpp Metro.cpp -o MetroApp.exe
```

#### Step 2: Run the CLI
```powershell
.\MetroApp.exe
```

---

## 5. REST API Reference

The C++ backend exposes the following REST endpoints on `http://127.0.0.1:8080`:

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/` | Serves `frontend/index.html` |
| `GET` | `/style.css` | Serves `frontend/style.css` |
| `GET` | `/script.js` | Serves `frontend/script.js` |
| `GET` | `/api/stations` | Returns JSON array of all stations, lines, and adjacency connections |
| `GET` | `/api/network` | Returns graph nodes and weighted edges for network map rendering |
| `GET` | `/api/route?from=X&to=Y` | Runs Dijkstra's algorithm, DP line optimization, and fare calculation in C++ |
| `GET` | `/api/bfs?start=X` | Runs Breadth-First Search from starting station in C++ |
| `GET` | `/api/dfs?start=X` | Runs Depth-First Search from starting station in C++ |
| `POST` | `/api/station` | Dynamically adds a new station to the C++ Graph in memory |
| `POST` | `/api/connection` | Dynamically adds an undirected weighted edge between two stations in C++ |

### Sample Route API Response:
```json
{
  "success": true,
  "message": "Shortest route computed successfully.",
  "source": "Karol Bagh",
  "destination": "Central Secretariat",
  "shortestPath": ["Karol Bagh", "Rajiv Chowk", "Patel Chowk", "Central Secretariat"],
  "totalDistance": 7,
  "fare": 25,
  "estimatedMinutes": 19,
  "interchangesCount": 1,
  "lineSegments": ["Blue Line", "Yellow Line", "Yellow Line"],
  "interchangeStations": [
    {
      "station": "Rajiv Chowk",
      "fromLine": "Blue Line",
      "toLine": "Yellow Line"
    }
  ],
  "dsaDetails": {
    "routeAlgorithm": "Dijkstra's Algorithm using Min-Heap Priority Queue O((V+E)log V)",
    "fareRule": "Tiered distance pricing: <=2km: Rs.10, >2km: Rs.10 + ceil((d-2)/2)*5",
    "interchangeAlgorithm": "Dynamic Programming Line Minimization O(Segments * Lines^2)"
  }
}
```

---

## 6. Technical Design Choices (College Project Context)

1. **Native Winsock2 vs Third-Party Frameworks**:
   - MinGW 32-bit compilers often lack POSIX `<thread>` implementations. By writing a direct Windows Sockets 2 server (`winsock2.h` with `-lws2_32`), the project runs with zero external package installations, compiles rapidly, and provides a clear demonstration of socket programming.
2. **Separation of Concerns**:
   - Shortest distance calculations (Dijkstra) and interchange transfer minimization (Dynamic Programming) are cleanly decoupled.
3. **No Algorithm Duplication**:
   - JavaScript is strictly used for presentation, DOM manipulation, and SVG map rendering. All algorithmic computations execute in compiled C++.

---

## 7. License & Credits

- Inspired by transit routing concepts and the Delhi Metro network.
- Developed as a comprehensive C++ Data Structures and Algorithms college project.
