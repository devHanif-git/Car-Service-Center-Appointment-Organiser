#include "menu.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"
#include "profile.h"
#include "appointment.h"
#include "service_bay.h"
#include "customer.h"
#include "vehicle.h"
#include "service_type.h"
#include "reports.h"
#include "staff.h"

// ============================================
// MAIN MENU
// ============================================
void mainMenu() {
    int choice;

    // =================================
    // ROLE: MECHANIC DASHBOARD
    // =================================
    if (currentUserRole == "Mechanic") {
        do {
            // Session timeout check
            if (isSessionExpired()) {
                showWarning("Session expired due to inactivity. Please login again.");
                pause();
                isLoggedIn = false;
                return;
            }

            setBreadcrumb("Home");
            clearScreen();
            displayHeader();
            displayBreadcrumb();
            cout << "\n\033[1;97m=== MECHANIC DASHBOARD: " << currentStaffName << " ===\033[0m" << endl;
            cout << "\n\033[36m1.\033[0m My Active Job List" << endl;
            cout << "\033[36m2.\033[0m View Bay Schedule" << endl;
            cout << "\033[36m3.\033[0m My Profile"  << endl << endl;
            cout << "\033[36m0.\033[0m Logout" << endl;

            try {
                choice = getValidInt("\nEnter choice", 0, 3);
                switch (choice) {
                case 1: setBreadcrumb("Home > Job List"); manageServiceJobDetails(); break;
                case 2: setBreadcrumb("Home > Bay Schedule"); checkBaySchedule(); break;
                case 3: setBreadcrumb("Home > My Profile"); viewMyProfile(); break;
                case 0: isLoggedIn = false; return;
                }
            }
            catch (OperationCancelledException&) {}
        } while (isLoggedIn);
    }

    // =================================
    // ROLE: ADMIN & MANAGER DASHBOARD
    // =================================
    else {
        // Auto-Generate Slots on Login (Manager Only)
        if (currentUserRole == "Manager") {
            autoGenerateFutureSlots(30);
            clearScreen();
        }

        do {
            // Session timeout check
            if (isSessionExpired()) {
                showWarning("Session expired due to inactivity. Please login again.");
                pause();
                isLoggedIn = false;
                return;
            }

            setBreadcrumb("Home");
            clearScreen();
            displayHeader();
            displayBreadcrumb();
            cout << "\n\033[1;97m=== MAIN DASHBOARD: " << currentStaffName << " (" << currentUserRole << ") ===\033[0m" << endl;

            cout << "\n\033[36m1.\033[0m My Profile" << endl;
            cout << "\033[36m2.\033[0m Service Operations" << endl;
            cout << "\033[36m3.\033[0m Bay & Facility Management" << endl;
            cout << "\033[36m4.\033[0m Customer Database" << endl;
            cout << "\033[36m5.\033[0m Vehicle Registry" << endl;
            cout << "\033[36m6.\033[0m Service Catalog" << endl;
            cout << "\033[36m7.\033[0m Analytics & Reports" << endl;

            if (currentUserRole == "Manager") {
                cout << "\033[36m8.\033[0m System Admin (Staff)" << endl;
            }

            cout << "\n\033[36m0.\033[0m Logout" << endl;

            try {
                int maxOpt = (currentUserRole == "Manager") ? 8 : 7;
                choice = getValidInt("\nEnter choice", 0, maxOpt);

                switch (choice) {
                case 1: setBreadcrumb("Home > My Profile"); viewMyProfile(); break;
                case 2: setBreadcrumb("Home > Service Operations"); scheduleAppointments(); break;
                case 3: setBreadcrumb("Home > Bay Management"); coordinateServiceBays(); break;
                case 4: setBreadcrumb("Home > Customers"); manageCustomerRecords(); break;
                case 5: setBreadcrumb("Home > Vehicles"); trackVehicleRecords(); break;
                case 6: setBreadcrumb("Home > Service Catalog"); maintainServiceTypes(); break;
                case 7: setBreadcrumb("Home > Reports"); generateReports(); break;
                case 8:
                    if (currentUserRole == "Manager") { setBreadcrumb("Home > Staff Admin"); manageStaff(); }
                    else showError("Restricted Access.");
                    break;
                case 0: isLoggedIn = false; return;
                }
            }
            catch (OperationCancelledException&) { choice = -1; }
        } while (isLoggedIn);
    }
}
