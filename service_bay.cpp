#include "service_bay.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// ADD SERVICE BAY
// ============================================
void addServiceBay() {
    clearScreen();
    printSectionTitle("ADD SERVICE BAY");

    try {
        string name = getValidString("Enter Bay Name (e.g., Bay 1)");

        // === DUPLICATION CHECK ===
        string checkQuery = "SELECT COUNT(*) FROM SERVICE_BAY WHERE bayName = '" + name + "'";
        if (mysql_query(conn, checkQuery.c_str())) {
            showError("DB Check Error: " + string(mysql_error(conn))); return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        int count = atoi(row[0]);
        mysql_free_result(res);

        if (count > 0) {
            showError("A bay named '" + name + "' already exists.");
            pause();
            return;
        }

        cout << "1. Available\n2. Occupied\n3. Maintenance\n";
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

            cout << "\n\033[1;97m=== New Bay Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << endl;
            mysql_free_result(res);
        }
    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// VIEW SERVICE BAYS
// ============================================
void viewServiceBays() {
    clearScreen();
    printSectionTitle("VIEW SERVICE BAYS");

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

    while (true) {
        clearScreen();
        printSectionTitle("VIEW SERVICE BAYS");

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT * FROM SERVICE_BAY ORDER BY serviceBayId LIMIT " +
            to_string(recordsPerPage) + " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        result = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Bay Name" << setw(20) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────" << "\033[0m" << endl;

        while ((row = mysql_fetch_row(result))) {
            cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(20) << row[2] << endl;
        }
        mysql_free_result(result);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " service bay(s)\033[0m" << endl;
        cout << "\n\033[36m[N]ext | [P]revious | [E]xit\033[0m" << endl;

        try {
            string input = getValidString("Enter choice", 1, 1, false);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) {
                currentPage++;
            }
            else if (choice == 'P' && currentPage > 1) {
                currentPage--;
            }
            else if (choice == 'E') {
                break;
            }
        }
        catch (OperationCancelledException&) { break; }
    }
}

// ============================================
// UPDATE SERVICE BAY STATUS
// ============================================
void updateServiceBayStatus() {
    clearScreen();
    printSectionTitle("UPDATE BAY STATUS");

    try {
        string query = "SELECT * FROM SERVICE_BAY";
        if (mysql_query(conn, query.c_str())) return;

        MYSQL_RES* res = mysql_store_result(conn);
        cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << endl;
        }
        mysql_free_result(res);

        int id = getValidInt("\nEnter Bay ID to update", 1, 999);

        cout << "\n1. Available\n2. Occupied\n3. Maintenance\n";
        int statusChoice = getValidInt("Select New Status", 1, 3);
        string statuses[] = { "", "Available", "Occupied", "Maintenance" };

        query = "UPDATE SERVICE_BAY SET bayStatus='" + statuses[statusChoice] + "' WHERE serviceBayId=" + to_string(id);

        if (mysql_query(conn, query.c_str()) == 0) {
            showSuccess("Status Updated Successfully!");

            query = "SELECT * FROM SERVICE_BAY WHERE serviceBayId=" + to_string(id);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            cout << "\n\033[1;97m=== Updated Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << endl;
            mysql_free_result(res);
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// DELETE SERVICE BAY
// ============================================
void deleteServiceBay() {
    clearScreen();
    printSectionTitle("DELETE SERVICE BAY");

    MYSQL_RES* result;
    MYSQL_ROW row;

    string query = "SELECT * FROM SERVICE_BAY ORDER BY serviceBayId";

    if (mysql_query(conn, query.c_str())) {
        showError("Error: " + string(mysql_error(conn)));
        pause();
        return;
    }

    result = mysql_store_result(conn);

    if (mysql_num_rows(result) == 0) {
        showWarning("No service bays found.");
        mysql_free_result(result);
        pause();
        return;
    }

    printSubHeader("Current Service Bays");
    cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Bay Name" << setw(20) << "Status" << "\033[0m" << endl;
    cout << "\033[90m" << u8"────────────────────────────────────────────────────" << "\033[0m" << endl;

    while ((row = mysql_fetch_row(result))) {
        cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(20) << row[2] << endl;
    }

    mysql_free_result(result);

    int bayId = getValidInt("\nEnter Service Bay ID to delete", 1, 999);

    if (bayId == -1) {
        return;
    }

    query = "SELECT COUNT(*) FROM APPOINTMENT WHERE serviceBayId = " + to_string(bayId) +
        " AND status IN ('Scheduled', 'In Progress')";

    if (mysql_query(conn, query.c_str())) {
        showError("Error: " + string(mysql_error(conn)));
        pause();
        return;
    }

    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);
    int activeAppointments = atoi(row[0]);
    mysql_free_result(result);

    if (activeAppointments > 0) {
        showError("WARNING: This bay has " + to_string(activeAppointments) + " active appointment(s)!");
        showError("Deleting will affect these appointments.");

        if (!getConfirmation("\nDo you want to proceed with deletion?")) {
            showWarning("Deletion cancelled.");
            pause();
            return;
        }
    }

    showError("WARNING: This action cannot be undone!");

    if (getConfirmation("Are you sure you want to delete this service bay?")) {
        query = "DELETE FROM SERVICE_BAY WHERE serviceBayId = " + to_string(bayId);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            showWarning("Note: Cannot delete bay if it has appointment records.");
            showWarning("Consider setting bay status to 'Maintenance' instead.");
        }
        else {
            if (mysql_affected_rows(conn) > 0) {
                showSuccess("Service Bay deleted successfully!");
            }
            else {
                showError("Service Bay ID not found!");
            }
        }
    }
    else {
        showWarning("Deletion cancelled.");
    }

    pause();
}

// ============================================
// CHECK BAY SCHEDULE
// ============================================
void checkBaySchedule() {
    clearScreen();
    printSectionTitle("CHECK BAY SCHEDULE");

    try {
        mysql_query(conn, "SELECT serviceBayId, bayName FROM SERVICE_BAY");
        MYSQL_RES* res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << "\033[0m" << endl;
        cout << "\033[90m" << u8"─────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) cout << left << setw(5) << row[0] << setw(20) << row[1] << endl;
        mysql_free_result(res);

        int bayId = getValidInt("\nEnter Bay ID to check", 1, 999);

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

        cout << "\n\033[1;97m=== Schedule & Gaps for Bay " << bayId << " (" << startDate << " to " << endDate << ") ===\033[0m" << endl;
        cout << "\033[90m(Standard Hours: 08:00:00 to 18:00:00)\033[0m" << endl;

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

                    cout << "\033[1;97m>>> DATE: " << rowDate << " <<<\033[0m" << endl;
                    currentProcessingDate = rowDate;
                    lastEnd = "08:00:00";
                }

                if (appStart > lastEnd) {
                    cout << "\033[32m[AVAILABLE] " << lastEnd << " --> " << appStart << "\033[0m" << endl;
                }

                cout << "\033[31m[ BUSY    ] " << appStart << " --> " << appEnd << "\033[0m" << endl;
                lastEnd = appEnd;
            }

            if (lastEnd < closingTime) {
                cout << "\033[32m[AVAILABLE] " << lastEnd << " --> " << closingTime << "\033[0m" << endl;
            }
        }
        mysql_free_result(res);

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// COORDINATE SERVICE BAYS MENU
// ============================================
void coordinateServiceBays() {
    int choice;
    do {
        clearScreen();
        displayHeader();
        cout << "\n\033[1;97m3.0 BAY & FACILITY MANAGEMENT\033[0m\n" << endl;
        cout << "\033[36m1.\033[0m Check Bay Schedule (Find Gaps)" << endl;
        cout << "\033[36m2.\033[0m View Bay Status Overview" << endl;
        cout << "\033[36m3.\033[0m Update Bay Status (Main./Occupied)" << endl;
        cout << "\033[36m4.\033[0m Add New Service Bay" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 4);
            switch (choice) {
            case 1: checkBaySchedule(); break;
            case 2: viewServiceBays(); break;
            case 3: updateServiceBayStatus(); break;
            case 4: addServiceBay(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause(); }
    } while (choice != 0);
}
