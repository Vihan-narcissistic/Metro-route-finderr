
#include "Metro.h"

int main() {

    Metro metro;
    int choice;

    do {
        cout << "\n";
        cout << "====================================\n";
        cout << "       METRO ROUTE FINDER (C++)     \n";
        cout << "====================================\n";

        
cout << "1. Display All Metro Stations\n";
cout << "2. Display Metro Network\n";
cout << "3. Find Shortest Route\n";
cout << "4. Add New Metro Station\n";
cout << "5. Add Metro Connection\n";
cout << "6. BFS Traversal\n";
cout << "7. DFS Traversal\n";
cout << "8. Calculate Metro Fare\n";
cout << "9. Display Interchange Stations\n";
cout << "10. Search Station Metro Lines\n";
cout << "11. Find Route with Interchanges\n";
cout << "0. Exit\n";


        cout << "====================================\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a number.\n";
            continue;
        }

        switch (choice) {

            case 1:
                metro.showStations();
                break;

            case 2:
                metro.showNetwork();
                break;

            case 3:
                metro.findShortestRoute();
                break;

            case 4:
                metro.addStation();
                break;

            case 5:
                metro.addConnection();
                break;

            case 6:
                metro.performBFS();
                break;

            case 7:
                metro.performDFS();
                break;

            case 8:
                metro.calculateFare();
                break;
            
            
            case 9:
                metro.displayInterchanges();
                break;

            case 10:
                metro.displayStationLines();
                break;
            case 11:
                metro.findRouteWithInterchanges();
                break;

            case 0:
                cout << "\nThank you for using Metro Route Finder!\n";
                break;

            default:
                cout << "\nInvalid choice! Try again.\n";
        }

    } while (choice != 0);

    return 0;
}
