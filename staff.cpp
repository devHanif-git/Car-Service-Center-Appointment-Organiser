#include "staff.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// ADD STAFF
// ============================================
void addStaff() {
    clearScreen();
    displayHeader();
    printSectionTitle("ADD NEW STAFF");

    try {
        printSubHeader("Enter Staff Details");
        string name = getValidString("Full Name");
        string username = getValidString("Username");

        // === DUPLICATION CHECK ===
        string checkQuery = "SELECT COUNT(*) FROM STAFF WHERE username = '" + username + "'";
        if (mysql_query(conn, checkQuery.c_str())) return;

        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        int count = atoi(row[0]);
        mysql_free_result(res);

        if (count > 0) {
            showError("The username '" + username + "' is already taken.");
            pause();
            return;
        }

        string password = getValidString("Password");
        string hashedPassword = hashPassword(password);

        cout << "\nSelect Role:\n1. Manager\n2. Admin\n3. Mechanic\n";
        int r = getValidInt("Choice", 1, 3);
        string roles[] = { "", "Manager", "Admin", "Mechanic" };

        string query = "INSERT INTO STAFF (username, password, fullName, role, isActive) VALUES ('" +
            username + "', '" + hashedPassword + "', '" + name + "', '" + roles[r] + "', 1)";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            showError("Username might already exist.");
        }
        else {
            int newId = mysql_insert_id(conn);
            showSuccess("Staff Added Successfully!");

            // Show New Record
            query = "SELECT staffId, fullName, username, role, isActive FROM STAFF WHERE staffId=" + to_string(newId);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            cout << "\n\033[1;97m=== New Staff Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Username" << setw(15) << "Role" << setw(10) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << setw(15) << row[3] << setw(10) << "\033[32mACTIVE\033[0m" << endl;
            mysql_free_result(res);
        }
    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// VIEW STAFF
// ============================================
void viewStaff() {
    clearScreen();
    printSectionTitle("VIEW STAFF LIST");

    // 1. Get Statistics
    string statsQuery = "SELECT "
        "COUNT(*) as total, "
        "SUM(CASE WHEN isActive = 1 THEN 1 ELSE 0 END) as active, "
        "SUM(CASE WHEN isActive = 0 THEN 1 ELSE 0 END) as inactive, "
        "SUM(CASE WHEN role = 'Manager' THEN 1 ELSE 0 END) as managers, "
        "SUM(CASE WHEN role = 'Admin' THEN 1 ELSE 0 END) as admins, "
        "SUM(CASE WHEN role = 'Mechanic' THEN 1 ELSE 0 END) as mechanics "
        "FROM STAFF";

    if (mysql_query(conn, statsQuery.c_str())) {
        showError("Error fetching stats: " + string(mysql_error(conn)));
        pause();
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    int totalStaff = atoi(row[0]);
    int totalActive = atoi(row[1]);
    int totalInactive = atoi(row[2]);
    int cntManager = atoi(row[3]);
    int cntAdmin = atoi(row[4]);
    int cntMechanic = atoi(row[5]);

    mysql_free_result(res);

    if (totalStaff == 0) {
        showError("No staff records found.");
        pause();
        return;
    }

    int recordsPerPage = 10;
    int totalPages = (totalStaff + recordsPerPage - 1) / recordsPerPage;
    int currentPage = 1;

    while (true) {
        clearScreen();
        printSectionTitle("VIEW STAFF LIST");

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT staffId, fullName, username, role, isActive FROM STAFF "
            "ORDER BY role, fullName LIMIT " + to_string(recordsPerPage) + " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            break;
        }

        res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Username" << setw(15) << "Role" << setw(10) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        while ((row = mysql_fetch_row(res))) {
            string status = (atoi(row[4]) == 1) ? "ACTIVE" : "INACTIVE";
            string statusDisplay = (status == "ACTIVE") ? "\033[32mACTIVE\033[0m" : "\033[33mINACTIVE\033[0m";

            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2]
                << setw(15) << row[3] << setw(10) << statusDisplay << endl;
        }
        mysql_free_result(res);

        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
        cout << "\033[90mPage " << currentPage << " of " << totalPages << " | Total Records: " << totalStaff << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
        cout << "\033[1;97mSummary:\033[0m" << endl;
        cout << "\033[90m   [Status] Active: " << totalActive << " | Inactive: " << totalInactive << "\033[0m" << endl;
        cout << "\033[90m   [Roles ] Managers: " << cntManager << " | Admins: " << cntAdmin << " | Mechanics: " << cntMechanic << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        cout << "\n\033[36m[N]ext | [P]revious | [E]xit\033[0m" << endl;

        try {
            string input = getValidString("Enter choice", 1, 1, false);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) { break; }
    }
}

// ============================================
// UPDATE STAFF
// ============================================
void updateStaff() {
    clearScreen();
    printSectionTitle("UPDATE STAFF DETAILS");

    try {
        string term = getValidString("Enter Name or Username to search");
        string query = "SELECT staffId, fullName, username, role FROM STAFF "
            "WHERE fullName LIKE '%" + term + "%' OR username LIKE '%" + term + "%'";

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { showError("No match found."); mysql_free_result(res); pause(); return; }

        cout << "\n\033[1;97m=== Matches ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Username" << setw(15) << "Role" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << setw(15) << row[3] << endl;
        }
        mysql_free_result(res);

        int id = getValidInt("\nEnter Staff ID", 1, 99999);

        if (id == currentStaffId) {
            showWarning("You cannot edit your own details here. Please go to 'My Profile'.");
            pause();
            return;
        }

        cout << "\n1. Full Name\n2. Role\n";
        int choice = getValidInt("Select Field", 1, 2);

        string updateQ = "UPDATE STAFF SET ";

        if (choice == 1) {
            updateQ += "fullName='" + getValidString("New Name") + "'";
        }
        else if (choice == 2) {
            cout << "\n\033[36mRoles: 1. Manager, 2. Admin, 3. Mechanic\033[0m\n";
            int r = getValidInt("Select Role", 1, 3);
            string roles[] = { "", "Manager", "Admin", "Mechanic" };
            updateQ += "role='" + roles[r] + "'";
        }

        updateQ += " WHERE staffId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            showSuccess("Staff Updated Successfully!");
        }
    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// DEACTIVATE STAFF
// ============================================
void deactivateStaff() {
    clearScreen();
    printSectionTitle("MANAGE STAFF STATUS");

    try {
        string term = getValidString("Enter Name or Username");
        string query = "SELECT staffId, fullName, username, role, isActive FROM STAFF "
            "WHERE fullName LIKE '%" + term + "%' OR username LIKE '%" + term + "%'";

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { showError("No match."); mysql_free_result(res); pause(); return; }

        cout << "\033[36m" << left << setw(5) << "ID" << setw(20) << "Name" << setw(15) << "Username" << setw(10) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"─────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            string status = (atoi(row[4]) == 1) ? "ACTIVE" : "INACTIVE";
            cout << left << setw(5) << row[0] << setw(20) << row[1] << setw(15) << row[2] << setw(10) << status << endl;
        }
        mysql_free_result(res);

        int id = getValidInt("\nEnter Staff ID to toggle status", 1, 99999);

        if (id == currentStaffId) {
            showWarning("ACTION DENIED: You cannot deactivate your own account while logged in!");
            pause();
            return;
        }

        query = "SELECT isActive, fullName FROM STAFF WHERE staffId=" + to_string(id);
        mysql_query(conn, query.c_str());
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        int currentStatus = atoi(row[0]);
        string name = row[1];
        mysql_free_result(res);

        string action = (currentStatus == 1) ? "DEACTIVATE" : "ACTIVATE";
        int newStatus = (currentStatus == 1) ? 0 : 1;

        if (getConfirmation("Are you sure you want to " + action + " user " + name + "?")) {
            query = "UPDATE STAFF SET isActive=" + to_string(newStatus) + " WHERE staffId=" + to_string(id);
            if (mysql_query(conn, query.c_str()) == 0) {
                showSuccess("Status updated successfully.");
            }
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// MANAGE STAFF MENU
// ============================================
void manageStaff() {
    int choice;
    do {
        clearScreen();
        displayHeader();
        cout << "\n\033[1;97m7.0 STAFF MANAGEMENT (MANAGER ONLY)\033[0m\n" << endl;
        cout << "\033[36m1.\033[0m Add New Staff" << endl;
        cout << "\033[36m2.\033[0m View Staff List" << endl;
        cout << "\033[36m3.\033[0m Update Staff Details" << endl;
        cout << "\033[36m4.\033[0m Manage Staff Status" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 4);
            switch (choice) {
            case 1: addStaff(); break;
            case 2: viewStaff(); break;
            case 3: updateStaff(); break;
            case 4: deactivateStaff(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; break; }
    } while (choice != 0);
}
