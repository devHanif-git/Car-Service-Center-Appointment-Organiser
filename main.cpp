// ============================================
// CAR SERVICE CENTER ORGANISER
// Main Entry Point
// ============================================

#include "globals.h"
#include "utils.h"
#include "database.h"
#include "auth.h"
#include "menu.h"
#include "ui_components.h"

// ============================================
// MAIN FUNCTION
// ============================================
int main() {
    initConsole();
    clearScreen();
    displayHeader();
    printSectionTitle("SYSTEM INITIALIZATION");
    cout << "\033[36mConnecting to database...\033[0m" << endl;

    if (!connectDatabase()) {
        printSubHeader("TROUBLESHOOTING");
        cout << "\033[33m1. Check if MySQL Server is running\033[0m" << endl;
        cout << "\033[33m2. Verify password in code\033[0m" << endl;
        cout << "\033[33m3. Ensure database 'car_service_db' exists\033[0m" << endl;
        cout << "\n\033[90mPress Enter to exit...\033[0m";
        cin.get();
        return 1;
    }

    while (true) {
        // A. Login Screen
        if (login()) {
            // B. Enter Role-Based Menu
            mainMenu();

            // C. Post-Logout Cleanup
            showSuccess("Logged out successfully.");

            // Reset variables for safety
            currentStaffId = 0;
            currentStaffName = "";
            currentUserRole = "";
            isLoggedIn = false;

            pause();
        }
        else {
            // Login failed 3 times or DB error
            break;
        }
    }

    // Close database connection
    mysql_close(conn);

    return 0;
}
