#include "service_type.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// ADD SERVICE TYPE
// ============================================
void addServiceType() {
    clearScreen();
    printSectionTitle("ADD SERVICE TYPE");
    cout << endl;

    try {
        printSubHeader("Enter Service Details");
        string name = getValidString("Enter Service Name");

        // === DUPLICATION CHECK ===
        string checkQuery = "SELECT COUNT(*) FROM SERVICE_TYPE WHERE serviceName = '" + name + "'";
        if (mysql_query(conn, checkQuery.c_str())) return;

        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        int count = atoi(row[0]);
        mysql_free_result(res);

        if (count > 0) {
            showError("Service '" + name + "' already exists.");
            pause();
            return;
        }

        int duration = getValidInt("Enter Standard Duration (minutes)", 5, 480);
        double price = (double)getValidInt("Enter Base Price (RM)", 1, 10000);
        string desc = getValidString("Enter Description");

        string query = "INSERT INTO SERVICE_TYPE (serviceName, standardDuration, serviceDescription, basePrice) "
            "VALUES ('" + name + "', " + to_string(duration) + ", '" + desc + "', " + to_string(price) + ")";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            clearScreen();
            printSectionTitle("ADD SERVICE TYPE");

            int newId = mysql_insert_id(conn);

            showSuccess("Service Type Added Successfully!");

            string confirmQ = "SELECT * FROM SERVICE_TYPE WHERE serviceTypeId = " + to_string(newId);
            if (mysql_query(conn, confirmQ.c_str()) == 0) {
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);

                cout << "\n=== New Service Record ===\033[0m" << endl;
                cout << left << setw(5) << "ID" << setw(25) << "Name" << setw(12) << "Duration" << setw(12) << "Price\033[0m" << endl;
                cout << "\033[90m" << u8"────────────────────────────────────────────────" << "\033[0m" << endl;
                cout << left << setw(5) << row[0] << setw(25) << row[1]
                    << setw(12) << (string(row[2]) + " min") << setw(12) << row[4] << endl;
                mysql_free_result(res);
            }
        }
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Services");
}

// ============================================
// SEARCH SERVICE TYPE
// ============================================
void searchServiceType() {
    clearScreen();
    printSectionTitle("SEARCH SERVICE TYPE");
    cout << endl;

    try {
        string term = getValidString("Enter Service Name or Description");

        string query = "SELECT * FROM SERVICE_TYPE WHERE serviceName LIKE '%" + term + "%' "
            "OR serviceDescription LIKE '%" + term + "%' ORDER BY serviceName";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            showError("No services found matching '" + term + "'");
        }
        else {
            cout << "\n\033[1;97m=== Search Results (" << num_rows << " found) ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Service Name"
                << setw(12) << "Duration" << setw(12) << "Price (RM)" << setw(35) << "Description" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                string id = row[0];
                string name = row[1];
                string duration = string(row[2]) + " min";
                string price = row[4];
                string desc = row[3] ? row[3] : u8"─";

                int colWidth = 35;
                int textMax = 32;
                int len = desc.length();

                if (len == 0) {
                    cout << left << setw(5) << id << setw(25) << name << setw(12) << duration
                        << setw(12) << price << setw(colWidth) << u8"─" << endl;
                    continue;
                }

                for (int i = 0; i < len; i += textMax) {
                    string chunk = desc.substr(i, textMax);

                    if (i == 0) {
                        cout << left << setw(5) << id
                            << setw(25) << name
                            << setw(12) << duration
                            << setw(12) << price
                            << setw(colWidth) << chunk << endl;
                    }
                    else {
                        cout << left << setw(5) << " "
                            << setw(25) << " "
                            << setw(12) << " "
                            << setw(12) << " "
                            << setw(colWidth) << chunk << endl;
                    }
                }
            }
        }
        mysql_free_result(result);
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Services");
}

// ============================================
// VIEW SERVICE TYPES
// ============================================
void viewServiceTypes() {
    clearScreen();
    printSectionTitle("VIEW SERVICE TYPES");
    cout << endl;

    MYSQL_RES* result;
    MYSQL_ROW row;

    int recordsPerPage = 10;
    int currentPage = 1;
    int totalRecords = 0;

    string countQuery = "SELECT COUNT(*) FROM SERVICE_TYPE";
    mysql_query(conn, countQuery.c_str());
    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);
    totalRecords = atoi(row[0]);
    mysql_free_result(result);

    if (totalRecords == 0) {
        showWarning("No service types found.");
        pause();
        return;
    }

    int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;

    // Redraw callback for paging
    auto redrawPage = [&]() {
        displayBreadcrumb();
        printSectionTitle("VIEW SERVICE TYPES");
        cout << endl;

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT * FROM SERVICE_TYPE ORDER BY serviceTypeId LIMIT " +
            to_string(recordsPerPage) + " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Service Name"
            << setw(12) << "Duration" << setw(12) << "Price (RM)" << setw(35) << "Description" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW r;
        while ((r = mysql_fetch_row(res))) {
            string id = r[0];
            string name = r[1];
            string duration = string(r[2]) + " min";
            string price = r[4];
            string desc = r[3] ? r[3] : u8"─";

            int colWidth = 35;
            int textMax = 32;
            size_t len = desc.length();

            if (len == 0) {
                cout << left << setw(5) << id << setw(25) << name << setw(12) << duration
                    << setw(12) << price << setw(colWidth) << u8"─" << endl;
                continue;
            }

            for (size_t i = 0; i < len; i += textMax) {
                string chunk = desc.substr(i, textMax);
                if (i == 0) {
                    cout << left << setw(5) << id << setw(25) << name << setw(12) << duration
                        << setw(12) << price << setw(colWidth) << chunk << endl;
                }
                else {
                    cout << left << setw(5) << " " << setw(25) << " " << setw(12) << " "
                        << setw(12) << " " << setw(colWidth) << chunk << endl;
                }
            }
        }
        mysql_free_result(res);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " service type(s)\033[0m" << endl;
        cout << "\n\033[36m[N]ext | [P]revious | [S]earch | [E]xit\033[0m" << endl;
    };

    while (true) {
        clearScreen();
        redrawPage();

        try {
            string input = getValidString("Enter choice", 1, 1, false, redrawPage);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'S') { searchServiceType(); return; }
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) {  pause(); break; }
    }
    setBreadcrumb("Home > Services");
}

// ============================================
// UPDATE SERVICE TYPE
// ============================================
void updateServiceType() {
    clearScreen();
    printSectionTitle("UPDATE SERVICE TYPE");
    cout << endl;

    try {
        string search = getValidString("Enter Service Name or Description to search");

        string query = "SELECT * FROM SERVICE_TYPE "
            "WHERE serviceName LIKE '%" + search + "%' "
            "OR serviceDescription LIKE '%" + search + "%' "
            "ORDER BY serviceName";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }

        MYSQL_RES* result = mysql_store_result(conn);
        if (mysql_num_rows(result) == 0) {
            showError("No service found."); mysql_free_result(result); pause(); return;
        }

        cout << "\n\033[1;97m=== Matching Services ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "No." << setw(25) << "Name" << setw(10) << "Price" << setw(35) << "Description" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        vector<int> serviceIds;
        while ((row = mysql_fetch_row(result))) {
            serviceIds.push_back(atoi(row[0]));
            string name = row[1];
            string price = row[4];
            string desc = row[3] ? row[3] : u8"─";

            int colWidth = 35;
            int textMax = 32;
            int len = desc.length();

            if (len == 0) {
                cout << left << setw(5) << serviceIds.size() << setw(25) << name << setw(10) << price << setw(colWidth) << u8"─" << endl;
                continue;
            }

            for (int i = 0; i < len; i += textMax) {
                string chunk = desc.substr(i, textMax);
                if (i == 0) {
                    cout << left << setw(5) << serviceIds.size() << setw(25) << name << setw(10) << price << setw(colWidth) << chunk << endl;
                }
                else {
                    cout << left << setw(5) << " " << setw(25) << " " << setw(10) << " " << setw(colWidth) << chunk << endl;
                }
            }
        }
        mysql_free_result(result);

        int serviceChoice = getValidInt("\nEnter Service No.", 1, (int)serviceIds.size());
        int id = serviceIds[serviceChoice - 1];

        // Fetch current values
        string fetchQ = "SELECT serviceName, standardDuration, basePrice, serviceDescription FROM SERVICE_TYPE WHERE serviceTypeId = " + to_string(id);
        if (mysql_query(conn, fetchQ.c_str())) { showError("Database error"); return; }
        MYSQL_RES* currRes = mysql_store_result(conn);
        MYSQL_ROW currRow = mysql_fetch_row(currRes);

        string currName = currRow[0];
        int currDuration = atoi(currRow[1]);
        int currPrice = atoi(currRow[2]);
        string currDesc = currRow[3] ? currRow[3] : "";
        mysql_free_result(currRes);

        cout << "\n1. Name\n2. Duration\n3. Price\n4. Description\n5. Update All\n";
        int choice = getValidInt("Select Field", 1, 5);

        string updateQ = "UPDATE SERVICE_TYPE SET ";
        string newStrVal;
        int newIntVal;

        if (choice == 1 || choice == 5) {
            while (true) {
                newStrVal = getValidString("New Name");
                if (newStrVal == currName) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "serviceName='" + newStrVal + "',";
        }
        if (choice == 2 || choice == 5) {
            while (true) {
                newIntVal = getValidInt("New Duration", 1, 480);
                if (newIntVal == currDuration) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "standardDuration=" + to_string(newIntVal) + ",";
        }
        if (choice == 3 || choice == 5) {
            while (true) {
                newIntVal = getValidInt("New Price", 0, 10000);
                if (newIntVal == currPrice) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "basePrice=" + to_string(newIntVal) + ",";
        }
        if (choice == 4 || choice == 5) {
            while (true) {
                newStrVal = getValidString("New Description");
                if (newStrVal == currDesc) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "serviceDescription='" + newStrVal + "',";
        }

        updateQ.pop_back();
        updateQ += " WHERE serviceTypeId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            clearScreen();
            printSectionTitle("UPDATE SERVICE TYPE");
            showSuccess("Service Updated Successfully!");

            string confirmQ = "SELECT * FROM SERVICE_TYPE WHERE serviceTypeId = " + to_string(id);
            mysql_query(conn, confirmQ.c_str());
            result = mysql_store_result(conn);

            cout << "\n\033[1;97m=== Updated Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name" << setw(12) << "Duration" << setw(12) << "Price" << setw(35) << "Description" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            if (row = mysql_fetch_row(result)) {
                string id = row[0];
                string name = row[1];
                string duration = string(row[2]) + " min";
                string price = row[4];
                string desc = row[3] ? row[3] : u8"─";

                int colWidth = 35;
                int textMax = 32;
                int len = desc.length();

                for (int i = 0; i < len; i += textMax) {
                    string chunk = desc.substr(i, textMax);
                    if (i == 0) {
                        cout << left << setw(5) << id << setw(25) << name << setw(12) << duration << setw(12) << price << setw(colWidth) << chunk << endl;
                    }
                    else {
                        cout << left << setw(5) << " " << setw(25) << " " << setw(12) << " " << setw(12) << " " << setw(colWidth) << chunk << endl;
                    }
                }
            }
            mysql_free_result(result);
        }
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Services");
}

// ============================================
// MAINTAIN SERVICE TYPES MENU
// ============================================
void maintainServiceTypes() {
    int choice;

    auto displayMenu = [&]() {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[36m1.\033[0m Search Service Price" << endl;
        cout << "\033[36m2.\033[0m View Full Catalog" << endl;
        cout << "\033[36m3.\033[0m Add New Service" << endl;
        cout << "\033[36m4.\033[0m Update Service Details" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;
    };

    do {
        displayMenu();

        try {
            choice = getMenuChoice("\nEnter choice", 0, 4, displayMenu);
            switch (choice) {
            case 1: setBreadcrumb("Home > Services > Search"); searchServiceType(); break;
            case 2: setBreadcrumb("Home > Services > View All"); viewServiceTypes(); break;
            case 3: setBreadcrumb("Home > Services > Add Service"); addServiceType(); break;
            case 4: setBreadcrumb("Home > Services > Update"); updateServiceType(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause();  break; }
    } while (choice != 0);
}
