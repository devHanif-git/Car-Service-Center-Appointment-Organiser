#include "service_bay.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// SYNC BAY CAPACITY TO SLOT_TIME
// ============================================
void syncBayCapacity() {
    // Count total non-maintenance bays
    string countQuery = "SELECT COUNT(*) FROM SERVICE_BAY WHERE bayStatus != 'Maintenance'";
    if (mysql_query(conn, countQuery.c_str())) return;

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    int availableBays = atoi(row[0]);
    mysql_free_result(res);

    // Update maxCapacity for all future slots
    string updateQuery = "UPDATE SLOT_TIME SET maxCapacity = " + to_string(availableBays) +
        " WHERE slotDate > CURDATE() OR (slotDate = CURDATE() AND slotTime > CURTIME())";
    mysql_query(conn, updateQuery.c_str());

    int affected = mysql_affected_rows(conn);
    if (affected > 0) {
        showInfo("Slot capacity synced: " + to_string(availableBays) + " bays available (" + to_string(affected) + " slots updated)");
    }
}

// ============================================
// ADD SERVICE BAY
// ============================================
void addServiceBay() {
    string name;
    MYSQL_RES* res;
    MYSQL_ROW row;

    try {
        // Loop until valid unique name is entered
        while (true) {
            clearScreen();
            displayBreadcrumb();
            printSectionTitle("ADD SERVICE BAY");
            cout << endl;

            name = getValidString("Enter Bay Name (e.g., Bay 1)");

            // === DUPLICATION CHECK ===
            string checkQuery = "SELECT COUNT(*) FROM SERVICE_BAY WHERE bayName = '" + name + "'";
            if (mysql_query(conn, checkQuery.c_str())) {
                showError("DB Check Error: " + string(mysql_error(conn))); return;
            }
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            int count = atoi(row[0]);
            mysql_free_result(res);

            if (count > 0) {
                showError("A bay named '" + name + "' already exists.");
                pause();
                continue;
            }
            break;
        }

        cout << "\n1. Available\n2. Occupied\n3. Maintenance\n";
        int statusChoice = getValidInt("Select Status", 1, 3);
        string statuses[] = { "", "Available", "Occupied", "Maintenance" };

        string query = "INSERT INTO SERVICE_BAY (bayName, bayStatus) VALUES ('" + name + "', '" + statuses[statusChoice] + "')";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            int newId = mysql_insert_id(conn);
            showSuccess("Service Bay Added Successfully!");

            string q = "SELECT * FROM SERVICE_BAY WHERE serviceBayId=" + to_string(newId);
            mysql_query(conn, q.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            printSectionDivider("New Bay Record", 38);
            cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"─────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << endl;
            mysql_free_result(res);

            // Sync slot capacity with new bay count
            syncBayCapacity();
        }
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Bays");
}

// ============================================
// VIEW SERVICE BAYS
// ============================================
void viewServiceBays() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("VIEW SERVICE BAYS");
    cout << endl;

    MYSQL_RES* result;
    MYSQL_ROW row;

    int recordsPerPage = 10;
    int currentPage = 1;
    int totalRecords = 0;

    string countQuery = "SELECT COUNT(*) FROM SERVICE_BAY";

    if (mysql_query(conn, countQuery.c_str())) {
        showError("Error: " + string(mysql_error(conn)));
        pause();
        return;
    }

    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);
    totalRecords = atoi(row[0]);
    mysql_free_result(result);

    if (totalRecords == 0) {
        showWarning("No service bays found.");
        pause();
        return;
    }

    int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;

    // Redraw callback for paging
    auto redrawPage = [&]() {
        displayBreadcrumb();
        printSectionTitle("VIEW SERVICE BAYS");
        cout << endl;

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT * FROM SERVICE_BAY ORDER BY serviceBayId LIMIT " +
            to_string(recordsPerPage) + " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Bay Name" << setw(20) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW r;
        while ((r = mysql_fetch_row(res))) {
            cout << left << setw(5) << r[0] << setw(25) << r[1] << setw(20) << r[2] << endl;
        }
        mysql_free_result(res);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " service bay(s)\033[0m" << endl;
        cout << "\n\033[36m[N]ext | [P]revious | [E]xit\033[0m" << endl;
    };

    while (true) {
        clearScreen();
        redrawPage();

        try {
            string input = getValidString("Enter choice", 1, 1, false, redrawPage);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) { pause();  break; }
    }
    setBreadcrumb("Home > Bays");
}

// ============================================
// UPDATE SERVICE BAY STATUS
// ============================================
void updateServiceBayStatus() {
    try {
        vector<int> bayIds;

        // Define redraw callback that fetches and displays bay list
        auto redrawBayList = [&bayIds]() {
            displayBreadcrumb();
            printSectionTitle("UPDATE BAY STATUS");
            cout << endl;

            bayIds.clear();
            string query = "SELECT * FROM SERVICE_BAY";
            if (mysql_query(conn, query.c_str())) return;

            MYSQL_RES* res = mysql_store_result(conn);
            cout << "\033[36m" << left << setw(5) << "No." << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────" << "\033[0m" << endl;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                bayIds.push_back(atoi(row[0]));
                cout << left << setw(5) << bayIds.size() << setw(20) << row[1] << setw(15) << row[2] << endl;
            }
            mysql_free_result(res);
        };

        // Initial display
        clearScreen();
        redrawBayList();

        if (bayIds.empty()) {
            showWarning("No service bays found.");
            pause();
            return;
        }

        int bayChoice = getValidInt("\nEnter Bay No.", 1, (int)bayIds.size(), redrawBayList);
        int id = bayIds[bayChoice - 1];

        // Fetch current status
        string fetchQ = "SELECT bayStatus FROM SERVICE_BAY WHERE serviceBayId = " + to_string(id);
        if (mysql_query(conn, fetchQ.c_str())) { showError("Database error"); return; }
        MYSQL_RES* currRes = mysql_store_result(conn);
        MYSQL_ROW currRow = mysql_fetch_row(currRes);
        string currStatus = currRow[0];
        mysql_free_result(currRes);

        cout << "\n1. Available\n2. Occupied\n3. Maintenance\n";
        string statuses[] = { "", "Available", "Occupied", "Maintenance" };
        int statusChoice;

        while (true) {
            statusChoice = getValidInt("Select New Status", 1, 3);
            if (statuses[statusChoice] == currStatus) {
                showWarning("New status cannot be the same as current status (" + currStatus + ").");
                continue;
            }
            break;
        }

        string query = "UPDATE SERVICE_BAY SET bayStatus='" + statuses[statusChoice] + "' WHERE serviceBayId=" + to_string(id);

        if (mysql_query(conn, query.c_str()) == 0) {
            showSuccess("Status Updated Successfully!");

            query = "SELECT * FROM SERVICE_BAY WHERE serviceBayId=" + to_string(id);
            mysql_query(conn, query.c_str());
            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row = mysql_fetch_row(res);

            printSectionDivider("Updated Record", 38);
            cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"─────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << endl;
            mysql_free_result(res);

            // Sync slot capacity when maintenance status changes
            syncBayCapacity();
        }
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Bays");
}

// ============================================
// CHECK BAY SCHEDULE
// ============================================
void checkBaySchedule() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("CHECK BAY SCHEDULE");
    cout << endl;

    try {
        // Only show operational bays (exclude Maintenance)
        mysql_query(conn, "SELECT serviceBayId, bayName, bayStatus FROM SERVICE_BAY WHERE bayStatus != 'Maintenance'");
        MYSQL_RES* res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "No." << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"─────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        vector<int> scheduleBayIds;
        while ((row = mysql_fetch_row(res))) {
            scheduleBayIds.push_back(atoi(row[0]));
            cout << left << setw(5) << scheduleBayIds.size() << setw(20) << row[1] << setw(15) << row[2] << endl;
        }
        mysql_free_result(res);

        if (scheduleBayIds.empty()) {
            showWarning("No operational bays found. All bays may be under maintenance.");
            pause();
            return;
        }

        int scheduleChoice = getValidInt("\nEnter Bay No.", 1, (int)scheduleBayIds.size());
        int bayId = scheduleBayIds[scheduleChoice - 1];

        string startDate = getSmartDateInput("Enter Start Date (YYYYMMDD)", false);
        string endDate = getSmartDateInput("Enter End Date (YYYYMMDD)  ", false);

        if (startDate > endDate) {
            showWarning("Invalid Range: Start Date cannot be after End Date.");
            pause();
            return;
        }

        string query = "SELECT st.slotDate, st.slotTime, a.endTime "
            "FROM APPOINTMENT a "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE a.serviceBayId = " + to_string(bayId) + " "
            "AND st.slotDate BETWEEN '" + startDate + "' AND '" + endDate + "' "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "ORDER BY st.slotDate ASC, st.slotTime ASC";

        if (mysql_query(conn, query.c_str())) return;
        res = mysql_store_result(conn);

        clearScreen();
        displayBreadcrumb();
        printSectionTitle("CHECK BAY SCHEDULE");
        cout << endl;

        cout << "\033[1;97m=== Schedule & Gaps for Bay " << bayId << " (" << startDate << " to " << endDate << ") ===\033[0m" << endl;
        cout << "\033[90m(Standard Hours: 08:00:00 to 18:00:00)\033[0m" << endl << endl;

        if (mysql_num_rows(res) == 0) {
            showSuccess("This Bay is COMPLETELY FREE for the selected range!");
        }
        else {
            string currentProcessingDate = "";
            string lastEnd = "08:00:00";
            const string closingTime = "18:00:00";

            while ((row = mysql_fetch_row(res))) {
                string rowDate = row[0];
                string appStart = row[1];
                string appEnd = row[2];

                if (rowDate != currentProcessingDate) {
                    if (!currentProcessingDate.empty()) {
                        if (lastEnd < closingTime) {
                            cout << "\033[32m[AVAILABLE] " << lastEnd << " --> " << closingTime << "\033[0m" << endl;
                        }
                        cout << endl;
                    }

                    printSubHeader("DATE: " + rowDate);
                    currentProcessingDate = rowDate;
                    lastEnd = "08:00:00";
                }

                if (appStart > lastEnd) {
                    cout << "\033[32m[AVAILABLE] " << lastEnd << " --> " << appStart << "\033[0m" << endl;
                }

                cout << "\033[31m[ B U S Y ] " << appStart << " --> " << appEnd << "\033[0m" << endl;
                lastEnd = appEnd;
            }

            if (lastEnd < closingTime) {
                cout << "\033[32m[AVAILABLE] " << lastEnd << " --> " << closingTime << "\033[0m" << endl;
            }
        }
        mysql_free_result(res);
        pause();

    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home");
}

// ============================================
// VIEW CURRENT BAY ACTIVITY (REAL-TIME)
// ============================================
void viewCurrentBayActivity() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("REAL-TIME BAY ACTIVITY");
    cout << endl;

    string query = "SELECT sb.serviceBayId, sb.bayName, sb.bayStatus, "
        "v.licensePlate, c.customerName, "
        "GROUP_CONCAT(DISTINCT svc.serviceName SEPARATOR ', ') as services, "
        "st.slotTime, a.endTime "
        "FROM SERVICE_BAY sb "
        "LEFT JOIN APPOINTMENT a ON sb.serviceBayId = a.serviceBayId "
        "    AND a.status IN ('Scheduled', 'In Progress') "
        "    AND a.slotTimeId IN (SELECT slotTimeId FROM SLOT_TIME WHERE slotDate = CURDATE()) "
        "LEFT JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
        "LEFT JOIN CUSTOMER c ON v.customerId = c.customerId "
        "LEFT JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
        "LEFT JOIN APPOINTMENT_SERVICE aps ON a.appointmentId = aps.appointmentId "
        "LEFT JOIN SERVICE_TYPE svc ON aps.serviceTypeId = svc.serviceTypeId "
        "GROUP BY sb.serviceBayId, sb.bayName, sb.bayStatus, v.licensePlate, c.customerName, st.slotTime, a.endTime "
        "ORDER BY sb.serviceBayId";

    if (mysql_query(conn, query.c_str())) {
        showError("Error: " + string(mysql_error(conn)));
        pause();
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;

    int availableCount = 0, occupiedCount = 0, maintenanceCount = 0;

    cout << "\033[1;97m" << u8"┌────────────────────────────────────────────────────────────────────────────────┐" << "\033[0m" << endl;
    cout << "\033[1;97m" << u8"│                         TODAY'S BAY STATUS                                     │" << "\033[0m" << endl;
    cout << "\033[1;97m" << u8"└────────────────────────────────────────────────────────────────────────────────┘" << "\033[0m" << endl;

    while ((row = mysql_fetch_row(res))) {
        string bayName = row[1];
        string status = row[2];
        string plate = (row[3] ? row[3] : "");
        string customer = (row[4] ? row[4] : "");
        string services = (row[5] ? row[5] : "");
        string startTime = (row[6] ? row[6] : "");
        string endTime = (row[7] ? row[7] : "");

        // Count stats
        if (status == "Available") availableCount++;
        else if (status == "Occupied") occupiedCount++;
        else if (status == "Maintenance") maintenanceCount++;

        // Status color and icon
        string statusIcon, statusColor;
        if (status == "Available") {
            statusIcon = u8"[AVAILABLE]";
            statusColor = "\033[32m";  // Green
        }
        else if (status == "Occupied") {
            statusIcon = u8"[OCCUPIED] ";
            statusColor = "\033[33m";  // Yellow
        }
        else {
            statusIcon = u8"[  MAINT  ]   ";
            statusColor = "\033[31m";  // Red
        }

        cout << "\n" << statusColor << statusIcon << "\033[0m \033[1;97m" << bayName << "\033[0m" << endl;

        if (!plate.empty()) {
            cout << "           " << u8"├─ Vehicle: \033[36m" << plate << "\033[0m (" << customer << ")" << endl;
            if (!services.empty()) {
                // Truncate long service list
                if (services.length() > 45) services = services.substr(0, 42) + "...";
                cout << "           " << u8"├─ Services: " << services << endl;
            }
            if (!startTime.empty()) {
                cout << "           " << u8"└─ Time: " << startTime << " - " << endTime << endl;
            }
        }
        else if (status == "Maintenance") {
            cout << "           " << u8"└─ Bay under maintenance" << endl;
        }
        else {
            cout << "           " << u8"└─ Ready for next vehicle" << endl;
        }
    }
    mysql_free_result(res);

    // Summary
    cout << "\n\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
    cout << "\033[1;97mSummary:\033[0m ";
    cout << "\033[32m" << availableCount << " Available\033[0m | ";
    cout << "\033[33m" << occupiedCount << " Occupied\033[0m | ";
    cout << "\033[31m" << maintenanceCount << " Maintenance\033[0m" << endl;

    setBreadcrumb("Home > Bays");
    pause();
}

// ============================================
// COORDINATE SERVICE BAYS MENU
// ============================================
void coordinateServiceBays() {
    int choice;

    auto displayMenu = [&]() {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[36m1.\033[0m Real-Time Bay Activity (Live)" << endl;
        cout << "\033[36m2.\033[0m Check Bay Schedule (Find Gaps)" << endl;
        cout << "\033[36m3.\033[0m View Bay Status Overview" << endl;
        cout << "\033[36m4.\033[0m Update Bay Status (Main./Occupied)" << endl;
        cout << "\033[36m5.\033[0m Add New Service Bay" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;
    };

    do {
        displayMenu();

        try {
            choice = getMenuChoice("\nEnter choice", 0, 5, displayMenu);
            switch (choice) {
            case 1: setBreadcrumb("Home > Bays > Live Activity"); viewCurrentBayActivity(); break;
            case 2: setBreadcrumb("Home > Bays > Schedule"); checkBaySchedule(); break;
            case 3: setBreadcrumb("Home > Bays > Status Overview"); viewServiceBays(); break;
            case 4: setBreadcrumb("Home > Bays > Update Status"); updateServiceBayStatus(); break;
            case 5: setBreadcrumb("Home > Bays > Add Bay"); addServiceBay(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause();  break; }
    } while (choice != 0);
}
