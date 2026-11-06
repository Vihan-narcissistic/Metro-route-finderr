// ============================================================================
// Metro Route Finder - Cross-Platform C++ HTTP Backend
// Supports: Windows (Winsock2) & Linux / Docker / Cloud (POSIX Sockets)
// ============================================================================

#ifdef _WIN32
  #define _WIN32_WINNT 0x0501
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <errno.h>

  typedef int SOCKET;
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1)
  #endif
  #ifndef SOCKET_ERROR
    #define SOCKET_ERROR (-1)
  #endif
  #ifndef closesocket
    #define closesocket close
  #endif
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include "Metro.h"

using namespace std;

// ----------------------------------------------------------------------------
// Cross-Platform Socket Helpers
// ----------------------------------------------------------------------------

inline bool initNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaErr != 0) {
        cerr << "[ERROR] WSAStartup failed with code: " << wsaErr << "\n";
        return false;
    }
#endif
    return true;
}

inline void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

inline int getSocketError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline void setSocketRecvTimeout(SOCKET sock, int milliseconds) {
#ifdef _WIN32
    DWORD timeout = static_cast<DWORD>(milliseconds);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv, sizeof(tv));
#endif
}

// ----------------------------------------------------------------------------
// HTTP & String Utilities
// ----------------------------------------------------------------------------

// URL decoding helper: converts %20 to space, + to space in query, %XX to byte
string urlDecode(const string& src) {
    string ret;
    ret.reserve(src.length());
    for (size_t i = 0; i < src.length(); i++) {
        if (src[i] == '%') {
            if (i + 2 < src.length() && isxdigit(src[i + 1]) && isxdigit(src[i + 2])) {
                int value = 0;
                istringstream is(src.substr(i + 1, 2));
                if (is >> hex >> value) {
                    ret += static_cast<char>(value);
                    i += 2;
                } else {
                    ret += src[i];
                }
            } else {
                ret += src[i];
            }
        } else if (src[i] == '+') {
            ret += ' ';
        } else {
            ret += src[i];
        }
    }
    return ret;
}

// JSON escaping helper
string jsonEscape(const string& s) {
    ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b";  break;
            case '\f': o << "\\f";  break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    o << "\\u00" << hex << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

// Simple query parameter parser
unordered_map<string, string> parseQueryParams(const string& queryStr) {
    unordered_map<string, string> params;
    size_t start = 0;
    while (start < queryStr.length()) {
        size_t amp = queryStr.find('&', start);
        if (amp == string::npos) amp = queryStr.length();
        string pair = queryStr.substr(start, amp - start);
        size_t eq = pair.find('=');
        if (eq != string::npos) {
            string key = urlDecode(pair.substr(0, eq));
            string val = urlDecode(pair.substr(eq + 1));
            params[key] = val;
        } else if (!pair.empty()) {
            params[urlDecode(pair)] = "";
        }
        start = amp + 1;
    }
    return params;
}

// Parse simple JSON or URL-encoded POST body key-values
unordered_map<string, string> parseBodyFields(const string& body) {
    unordered_map<string, string> fields;
    size_t first = body.find_first_not_of(" \t\r\n");
    if (first != string::npos && body[first] == '{') {
        size_t i = first + 1;
        while (i < body.length()) {
            size_t kStart = body.find('"', i);
            if (kStart == string::npos) break;
            size_t kEnd = body.find('"', kStart + 1);
            if (kEnd == string::npos) break;
            string key = body.substr(kStart + 1, kEnd - kStart - 1);

            size_t colon = body.find(':', kEnd + 1);
            if (colon == string::npos) break;

            size_t valStart = body.find_first_not_of(" \t\r\n", colon + 1);
            if (valStart == string::npos) break;

            string value;
            if (body[valStart] == '"') {
                size_t valEnd = body.find('"', valStart + 1);
                if (valEnd == string::npos) break;
                value = body.substr(valStart + 1, valEnd - valStart - 1);
                i = valEnd + 1;
            } else {
                size_t valEnd = body.find_first_of(",}\r\n", valStart);
                if (valEnd == string::npos) valEnd = body.length();
                value = body.substr(valStart, valEnd - valStart);
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                i = valEnd;
            }
            fields[key] = value;
        }
    } else {
        fields = parseQueryParams(body);
    }
    return fields;
}

// Read binary/text file from disk
bool readFile(const string& path, string& content) {
    ifstream file(path.c_str(), ios::in | ios::binary);
    if (!file.is_open()) return false;
    ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();
    return true;
}

// Send standard HTTP response
void sendHttpResponse(SOCKET client, int statusCode, const string& statusText,
                      const string& contentType, const string& body) {
    ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    string respStr = response.str();
    send(client, respStr.c_str(), static_cast<int>(respStr.length()), 0);
}

// ----------------------------------------------------------------------------
// Main Server Entrypoint
// ----------------------------------------------------------------------------

int main() {
    // 1. Initialize Network Subsystem
    if (!initNetwork()) {
        return 1;
    }

    // 2. Determine Port: Read from PORT env var, default to 8080
    int port = 8080;
    const char* envPort = getenv("PORT");
    if (envPort != nullptr && strlen(envPort) > 0) {
        int parsedPort = atoi(envPort);
        if (parsedPort > 0 && parsedPort <= 65535) {
            port = parsedPort;
        }
    }

    // 3. Create Server Socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "[ERROR] socket() creation failed: " << getSocketError() << "\n";
        cleanupNetwork();
        return 1;
    }

    // Enable SO_REUSEADDR for rapid restarts
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
#endif

    // 4. Bind to 0.0.0.0 (All interfaces, enabling Docker & Cloud port routing)
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[ERROR] bind() failed on 0.0.0.0:" << port << " (Error code: " << getSocketError() << ")\n";
        closesocket(serverSocket);
        cleanupNetwork();
        return 1;
    }

    // 5. Listen for incoming client connections
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        cerr << "[ERROR] listen() failed (Error code: " << getSocketError() << ")\n";
        closesocket(serverSocket);
        cleanupNetwork();
        return 1;
    }

    // 6. Display Startup Banner
    cout << "========================================================\n";
    cout << "   METRO ROUTE FINDER - C++ HTTP BACKEND\n";
#ifdef _WIN32
    cout << "   [Platform: Windows (Winsock2)]\n";
#else
    cout << "   [Platform: Linux / POSIX Sockets (Docker / Cloud)]\n";
#endif
    cout << "========================================================\n";
    cout << " [OK] Server listening on port " << port << " (bound to 0.0.0.0)\n";
    cout << " [OK] Local access URL : http://localhost:" << port << "\n";
    cout << " [OK] Cloud / Docker   : http://0.0.0.0:" << port << "\n";
    cout << " [OK] Core DSA Algorithms running inside native C++:\n";
    cout << "      - Dijkstra's Algorithm (Min-Heap Priority Queue)\n";
    cout << "      - Breadth First Search (Queue)\n";
    cout << "      - Depth First Search (Stack)\n";
    cout << "      - Fare Calculation & Transfer Minimization DP\n";
    cout << "========================================================\n";
    cout << " Press Ctrl+C in this terminal to stop the server.\n";
    cout << "--------------------------------------------------------\n";

    Metro metro;

    // 7. Request Processing Loop
    while (true) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        // Set 2.5 second timeout on receive
        setSocketRecvTimeout(clientSocket, 2500);

        // Receive full HTTP request including body if Content-Length specified
        string requestStr;
        vector<char> chunk(4096);
        size_t contentLength = 0;
        bool headerComplete = false;
        size_t headerEndPos = string::npos;

        while (true) {
            int n = recv(clientSocket, chunk.data(), static_cast<int>(chunk.size()), 0);
            if (n <= 0) break;
            requestStr.append(chunk.data(), n);

            if (!headerComplete) {
                headerEndPos = requestStr.find("\r\n\r\n");
                if (headerEndPos != string::npos) {
                    headerComplete = true;

                    // Check for Expect: 100-continue
                    string lowerReq = requestStr;
                    transform(lowerReq.begin(), lowerReq.end(), lowerReq.begin(), ::tolower);
                    if (lowerReq.find("expect: 100-continue") != string::npos) {
                        const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
                        send(clientSocket, cont, static_cast<int>(strlen(cont)), 0);
                    }

                    // Check Content-Length
                    size_t clPos = lowerReq.find("content-length:");
                    if (clPos != string::npos && clPos < headerEndPos) {
                        size_t valStart = lowerReq.find_first_not_of(" \t", clPos + 15);
                        size_t valEnd = lowerReq.find_first_of("\r\n", valStart);
                        if (valStart != string::npos && valEnd != string::npos) {
                            contentLength = static_cast<size_t>(atoi(lowerReq.substr(valStart, valEnd - valStart).c_str()));
                        }
                    }
                }
            }

            if (headerComplete) {
                size_t bodyBytesSoFar = requestStr.length() - (headerEndPos + 4);
                if (bodyBytesSoFar >= contentLength) {
                    break;
                }
            }
        }

        if (requestStr.empty()) {
            closesocket(clientSocket);
            continue;
        }

        // Parse request line
        istringstream reqStream(requestStr);
        string method, fullPath, httpVer;
        reqStream >> method >> fullPath >> httpVer;

        // Separate path and query string
        string path = fullPath;
        string queryStr;
        size_t qPos = fullPath.find('?');
        if (qPos != string::npos) {
            path = fullPath.substr(0, qPos);
            queryStr = fullPath.substr(qPos + 1);
        }

        // Handle CORS preflight OPTIONS request
        if (method == "OPTIONS") {
            sendHttpResponse(clientSocket, 200, "OK", "text/plain", "");
            closesocket(clientSocket);
            continue;
        }

        // Favicon handler
        if (path == "/favicon.ico") {
            string svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><text y='20' font-size='20'>🚇</text></svg>";
            sendHttpResponse(clientSocket, 200, "OK", "image/svg+xml", svg);
            closesocket(clientSocket);
            continue;
        }

        // Extract body if POST request
        string body;
        if (headerEndPos != string::npos && headerEndPos + 4 <= requestStr.length()) {
            body = requestStr.substr(headerEndPos + 4);
        }

        // --------------------------------------------------------------------
        // API Route: GET /api/stations
        // --------------------------------------------------------------------
        if (path == "/api/stations" && method == "GET") {
            vector<StationInfo> stations = metro.getAllStationDetails();
            ostringstream json;
            json << "{\"success\":true,\"count\":" << stations.size() << ",\"stations\":[";
            for (size_t i = 0; i < stations.size(); i++) {
                if (i > 0) json << ",";
                json << "{\"name\":\"" << jsonEscape(stations[i].name) << "\",";
                json << "\"isInterchange\":" << (stations[i].isInterchange ? "true" : "false") << ",";
                json << "\"lines\":[";
                for (size_t j = 0; j < stations[i].lines.size(); j++) {
                    if (j > 0) json << ",";
                    json << "\"" << jsonEscape(stations[i].lines[j]) << "\"";
                }
                json << "],\"connections\":[";
                for (size_t k = 0; k < stations[i].connections.size(); k++) {
                    if (k > 0) json << ",";
                    json << "{\"station\":\"" << jsonEscape(stations[i].connections[k].first) << "\",\"distance\":"
                         << stations[i].connections[k].second << "}";
                }
                json << "]}";
            }
            json << "]}";
            sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8", json.str());
        }
        // --------------------------------------------------------------------
        // API Route: GET /api/network
        // --------------------------------------------------------------------
        else if (path == "/api/network" && method == "GET") {
            vector<StationInfo> stations = metro.getAllStationDetails();
            ostringstream json;
            json << "{\"success\":true,\"nodes\":[";
            for (size_t i = 0; i < stations.size(); i++) {
                if (i > 0) json << ",";
                json << "{\"id\":\"" << jsonEscape(stations[i].name) << "\",\"name\":\"" << jsonEscape(stations[i].name) << "\",";
                json << "\"isInterchange\":" << (stations[i].isInterchange ? "true" : "false") << ",";
                json << "\"lines\":[";
                for (size_t j = 0; j < stations[i].lines.size(); j++) {
                    if (j > 0) json << ",";
                    json << "\"" << jsonEscape(stations[i].lines[j]) << "\"";
                }
                json << "]}";
            }
            json << "],\"edges\":[";
            bool firstEdge = true;
            for (size_t i = 0; i < stations.size(); i++) {
                for (size_t k = 0; k < stations[i].connections.size(); k++) {
                    if (stations[i].name < stations[i].connections[k].first) {
                        if (!firstEdge) json << ",";
                        firstEdge = false;
                        string u = stations[i].name;
                        string v = stations[i].connections[k].first;
                        string commonLine = "Standard Line";
                        auto it1 = metro.getStationLines().find(u);
                        auto it2 = metro.getStationLines().find(v);
                        if (it1 != metro.getStationLines().end() && it2 != metro.getStationLines().end()) {
                            for (const string& l1 : it1->second) {
                                if (find(it2->second.begin(), it2->second.end(), l1) != it2->second.end()) {
                                    commonLine = l1;
                                    break;
                                }
                            }
                        }
                        json << "{\"source\":\"" << jsonEscape(u) << "\",\"target\":\""
                             << jsonEscape(v) << "\",\"distance\":"
                             << stations[i].connections[k].second
                             << ",\"line\":\"" << jsonEscape(commonLine) << "\"}";
                    }
                }
            }
            json << "]}";
            sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8", json.str());
        }
        // --------------------------------------------------------------------
        // API Route: GET /api/route?from=X&to=Y
        // --------------------------------------------------------------------
        else if (path == "/api/route" && method == "GET") {
            unordered_map<string, string> params = parseQueryParams(queryStr);
            string from = params["from"];
            string to = params["to"];

            if (from.empty() || to.empty()) {
                ostringstream err;
                err << "{\"success\":false,\"message\":\"Parameters 'from' and 'to' are required.\"}";
                sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8", err.str());
            } else {
                RouteResult res = metro.getRoute(from, to);
                ostringstream json;
                if (!res.success) {
                    json << "{\"success\":false,\"message\":\"" << jsonEscape(res.message) << "\"}";
                    sendHttpResponse(clientSocket, 404, "Not Found", "application/json; charset=utf-8", json.str());
                } else {
                    int estimatedMins = (res.distance == 0) ? 0 : (res.distance * 2 + res.interchanges * 5);

                    json << "{\"success\":true,\"message\":\"" << jsonEscape(res.message) << "\",";
                    json << "\"source\":\"" << jsonEscape(from) << "\",";
                    json << "\"destination\":\"" << jsonEscape(to) << "\",";

                    // Section 1: Shortest-distance route calculation (Dijkstra)
                    json << "\"shortestPath\":[";
                    for (size_t i = 0; i < res.path.size(); i++) {
                        if (i > 0) json << ",";
                        json << "\"" << jsonEscape(res.path[i]) << "\"";
                    }
                    json << "],";
                    json << "\"totalDistance\":" << res.distance << ",";
                    json << "\"fare\":" << res.fare << ",";
                    json << "\"estimatedMinutes\":" << estimatedMins << ",";

                    // Section 2: Interchange & line optimization (Dynamic Programming)
                    json << "\"interchangesCount\":" << res.interchanges << ",";
                    json << "\"lineSegments\":[";
                    for (size_t i = 0; i < res.lineSegments.size(); i++) {
                        if (i > 0) json << ",";
                        json << "\"" << jsonEscape(res.lineSegments[i]) << "\"";
                    }
                    json << "],";
                    json << "\"interchangeStations\":[";
                    for (size_t i = 0; i < res.interchangeList.size(); i++) {
                        if (i > 0) json << ",";
                        json << "{\"station\":\"" << jsonEscape(res.interchangeList[i].station) << "\",";
                        json << "\"fromLine\":\"" << jsonEscape(res.interchangeList[i].fromLine) << "\",";
                        json << "\"toLine\":\"" << jsonEscape(res.interchangeList[i].toLine) << "\"}";
                    }
                    json << "],";

                    // Section 3: Segment details
                    json << "\"routeDetails\":[";
                    for (size_t i = 0; i < res.lineSegments.size(); i++) {
                        if (i > 0) json << ",";
                        int segDist = metro.getGraph().getShortestDistance(res.path[i], res.path[i + 1]);
                        json << "{\"from\":\"" << jsonEscape(res.path[i]) << "\",";
                        json << "\"to\":\"" << jsonEscape(res.path[i + 1]) << "\",";
                        json << "\"line\":\"" << jsonEscape(res.lineSegments[i]) << "\",";
                        json << "\"distance\":" << segDist << "}";
                    }
                    json << "],";

                    // Technical explanation metadata
                    json << "\"dsaDetails\":{";
                    json << "\"routeAlgorithm\":\"Dijkstra's Algorithm using Min-Heap Priority Queue O((V+E)log V)\",";
                    json << "\"fareRule\":\"Tiered distance pricing: <=2km: Rs.10, >2km: Rs.10 + ceil((d-2)/2)*5\",";
                    json << "\"interchangeAlgorithm\":\"Dynamic Programming Line Minimization O(Segments * Lines^2)\"";
                    json << "}}";

                    sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8", json.str());
                }
            }
        }
        // --------------------------------------------------------------------
        // API Route: GET /api/bfs?start=X
        // --------------------------------------------------------------------
        else if (path == "/api/bfs" && method == "GET") {
            unordered_map<string, string> params = parseQueryParams(queryStr);
            string start = params["start"];
            if (start.empty()) {
                sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                 "{\"success\":false,\"message\":\"'start' station parameter is required.\"}");
            } else {
                vector<string> traversal = metro.getBFSOrder(start);
                if (traversal.empty()) {
                    sendHttpResponse(clientSocket, 404, "Not Found", "application/json; charset=utf-8",
                                     "{\"success\":false,\"message\":\"Station does not exist.\"}");
                } else {
                    ostringstream json;
                    json << "{\"success\":true,\"start\":\"" << jsonEscape(start) << "\",";
                    json << "\"algorithm\":\"Breadth First Search (BFS)\",";
                    json << "\"dataStructure\":\"Queue (FIFO)\",";
                    json << "\"timeComplexity\":\"O(V + E)\",";
                    json << "\"explanation\":\"BFS visits neighbor stations level by level using a FIFO queue.\",";
                    json << "\"traversal\":[";
                    for (size_t i = 0; i < traversal.size(); i++) {
                        if (i > 0) json << ",";
                        json << "\"" << jsonEscape(traversal[i]) << "\"";
                    }
                    json << "]}";
                    sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8", json.str());
                }
            }
        }
        // --------------------------------------------------------------------
        // API Route: GET /api/dfs?start=X
        // --------------------------------------------------------------------
        else if (path == "/api/dfs" && method == "GET") {
            unordered_map<string, string> params = parseQueryParams(queryStr);
            string start = params["start"];
            if (start.empty()) {
                sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                 "{\"success\":false,\"message\":\"'start' station parameter is required.\"}");
            } else {
                vector<string> traversal = metro.getDFSOrder(start);
                if (traversal.empty()) {
                    sendHttpResponse(clientSocket, 404, "Not Found", "application/json; charset=utf-8",
                                     "{\"success\":false,\"message\":\"Station does not exist.\"}");
                } else {
                    ostringstream json;
                    json << "{\"success\":true,\"start\":\"" << jsonEscape(start) << "\",";
                    json << "\"algorithm\":\"Depth First Search (DFS)\",";
                    json << "\"dataStructure\":\"Stack (LIFO)\",";
                    json << "\"timeComplexity\":\"O(V + E)\",";
                    json << "\"explanation\":\"DFS traverses deep down each metro line branch before backtracking using a LIFO stack.\",";
                    json << "\"traversal\":[";
                    for (size_t i = 0; i < traversal.size(); i++) {
                        if (i > 0) json << ",";
                        json << "\"" << jsonEscape(traversal[i]) << "\"";
                    }
                    json << "]}";
                    sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8", json.str());
                }
            }
        }
        // --------------------------------------------------------------------
        // API Route: POST /api/station
        // --------------------------------------------------------------------
        else if (path == "/api/station" && method == "POST") {
            unordered_map<string, string> fields = parseBodyFields(body);
            string name = fields["name"];
            string line = fields["line"];
            if (name.empty()) {
                sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                 "{\"success\":false,\"message\":\"Station 'name' is required.\"}");
            } else {
                vector<string> lines;
                if (!line.empty()) lines.push_back(line);
                bool ok = metro.addStationDirect(name, lines);
                if (ok) {
                    sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8",
                                     "{\"success\":true,\"message\":\"Station added successfully.\"}");
                } else {
                    sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                     "{\"success\":false,\"message\":\"Station already exists or name invalid.\"}");
                }
            }
        }
        // --------------------------------------------------------------------
        // API Route: POST /api/connection
        // --------------------------------------------------------------------
        else if (path == "/api/connection" && method == "POST") {
            unordered_map<string, string> fields = parseBodyFields(body);
            string s1 = fields["station1"];
            string s2 = fields["station2"];
            string distStr = fields["distance"];
            int dist = 0;
            if (!distStr.empty()) dist = atoi(distStr.c_str());

            if (s1.empty() || s2.empty() || dist <= 0) {
                sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                 "{\"success\":false,\"message\":\"station1, station2 and a positive distance are required.\"}");
            } else {
                bool ok = metro.addConnectionDirect(s1, s2, dist);
                if (ok) {
                    sendHttpResponse(clientSocket, 200, "OK", "application/json; charset=utf-8",
                                     "{\"success\":true,\"message\":\"Connection added successfully.\"}");
                } else {
                    sendHttpResponse(clientSocket, 400, "Bad Request", "application/json; charset=utf-8",
                                     "{\"success\":false,\"message\":\"Failed to add connection. Verify stations exist and distance > 0.\"}");
                }
            }
        }
        // --------------------------------------------------------------------
        // Static File Serving from frontend/ directory
        // --------------------------------------------------------------------
        else {
            string filePath = "./frontend";
            string contentType = "text/plain";

            if (path == "/" || path == "/index.html") {
                filePath += "/index.html";
                contentType = "text/html; charset=utf-8";
            } else if (path == "/style.css") {
                filePath += "/style.css";
                contentType = "text/css; charset=utf-8";
            } else if (path == "/script.js") {
                filePath += "/script.js";
                contentType = "application/javascript; charset=utf-8";
            } else {
                filePath += path;
                if (path.rfind(".html") != string::npos) contentType = "text/html; charset=utf-8";
                else if (path.rfind(".css") != string::npos) contentType = "text/css; charset=utf-8";
                else if (path.rfind(".js") != string::npos) contentType = "application/javascript; charset=utf-8";
                else if (path.rfind(".svg") != string::npos) contentType = "image/svg+xml";
                else if (path.rfind(".json") != string::npos) contentType = "application/json";
            }

            string content;
            if (readFile(filePath, content)) {
                sendHttpResponse(clientSocket, 200, "OK", contentType, content);
            } else {
                string notFound = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><p>Resource " + path + " not found.</p></body></html>";
                sendHttpResponse(clientSocket, 404, "Not Found", "text/html; charset=utf-8", notFound);
            }
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    cleanupNetwork();
    return 0;
}
