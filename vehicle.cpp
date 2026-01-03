#include "vehicle.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// ADD VEHICLE
// ============================================
void addVehicle() {
    clearScreen();
    printSectionTitle("ADD NEW VEHICLE");

    try {
        string searchTerm = getValidString("Enter Customer Name, Phone, or Email to search");

        string query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
            "WHERE (customerName LIKE '%" + searchTerm + "%' "
            "OR phoneNumber LIKE '%" + searchTerm + "%' "
            "OR email LIKE '%" + searchTerm + "%')";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            showError("No active customer found matching '" + searchTerm + "'");
            cout << "Please add the customer first.\033[0m" << endl;
            mysql_free_result(result);
            pause();
            return;
        }

        cout << "\n\033[1;97m=== Matching Customers ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name" << setw(15) << "Phone" << setw(25) << "Email" << "\033[0m" << endl;
        cout << "\033[90m" << u8"──────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(15) << row[2] << setw(25) << row[3] << endl;
        }
        mysql_free_result(result);

        int customerId = getValidInt("\nEnter Customer ID from the list above", 1, 99999);

        // Verify customer exists
        query = "SELECT customerId, customerName FROM CUSTOMER WHERE customerId = " + to_string(customerId);
        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn))); return;
        }
        result = mysql_store_result(conn);
        if (mysql_num_rows(result) == 0) {
            showError("Invalid Customer ID.");
            mysql_free_result(result);
            pause();
            return;
        }
        row = mysql_fetch_row(result);
        string custName = row[1];
        mysql_free_result(result);

        displayCustomerVehicles(customerId);

        // Loop to add vehicles
        while (true) {
            cout << "\n\033[1;97m=== Add New Vehicle for " << custName << " ===\033[0m" << endl;

            string licensePlate = getValidString("Enter License Plate");

            // === DUPLICATION CHECK ===
            string checkQuery = "SELECT COUNT(*) FROM VEHICLE WHERE licensePlate = '" + licensePlate + "'";
            if (mysql_query(conn, checkQuery.c_str())) return;

            MYSQL_RES* res = mysql_store_result(conn);
            MYSQL_ROW row = mysql_fetch_row(res);
            int count = atoi(row[0]);
            mysql_free_result(res);

            if (count > 0) {
                showError("Vehicle with plate '" + licensePlate + "' is already registered.");
                if (!getConfirmation("Do you want to add a different vehicle?")) break;
                continue;
            }

            string brand = getValidString("Enter Brand");
            string model = getValidString("Enter Model");
            string year = getValidYear("Enter Year");
            string color = getValidString("Enter Color");

            query = "INSERT INTO VEHICLE (customerId, licensePlate, brand, model, year, color) "
                "VALUES (" + to_string(customerId) + ", '" + licensePlate + "', '" +
                brand + "', '" + model + "', '" + year + "', '" + color + "')";

            if (mysql_query(conn, query.c_str())) {
                showError("Error adding vehicle: " + string(mysql_error(conn)));
            }
            else {
                showSuccess("Vehicle Added Successfully!");
                cout << "\033[90m    License : " << licensePlate << "\033[0m" << endl;
                cout << "\033[90m    Vehicle : " << brand << " " << model << " (" << color << ")\033[0m" << endl;

                showSuccess("Updating vehicle list...");
                displayCustomerVehicles(customerId);
            }

            if (!getConfirmation("\nAdd another vehicle for this customer?")) {
                break;
            }
        }

    }
    catch (OperationCancelledException& e) {
    }

    pause();
}

// ============================================
// SEARCH VEHICLE
// ============================================
void searchVehicle() {
    clearScreen();
    printSectionTitle("VEHICLE SEARCH");

    try {
        string term = getValidString("Enter Plate, Brand, Model, or Owner Name");

        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, v.year, c.customerName, c.phoneNumber "
            "FROM VEHICLE v "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE v.licensePlate LIKE '%" + term + "%' "
            "OR v.brand LIKE '%" + term + "%' "
            "OR v.model LIKE '%" + term + "%' "
            "OR c.customerName LIKE '%" + term + "%' "
            "ORDER BY v.licensePlate";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            showError("No vehicles found matching '" + term + "'");
        }
        else {
            cout << "\n\033[1;97m=== Search Results (" << num_rows << " found) ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(12) << "Brand"
                << setw(12) << "Model" << setw(25) << "Owner" << setw(15) << "Phone" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                cout << left << setw(5) << row[0] << setw(15) << row[1] << setw(12) << row[2]
                    << setw(12) << row[3] << setw(25) << row[5] << setw(15) << row[6] << endl;
            }
        }
        mysql_free_result(result);

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// VIEW VEHICLES
// ============================================
void viewVehicles() {
    clearScreen();
    printSectionTitle("VIEW ALL VEHICLES");

    MYSQL_RES* result;
    MYSQL_ROW row;

    int recordsPerPage = 10;
    int currentPage = 1;
    int totalRecords = 0;

    string countQuery = "SELECT COUNT(*) FROM VEHICLE v JOIN CUSTOMER c ON v.customerId = c.customerId";
    if (mysql_query(conn, countQuery.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }

    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);
    totalRecords = atoi(row[0]);
    mysql_free_result(result);

    if (totalRecords == 0) {
        showWarning("No vehicles found for active customers.");
        pause();
        return;
    }

    int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;

    while (true) {
        clearScreen();
        printSectionTitle("VIEW ALL VEHICLES");

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, v.year, c.customerName "
            "FROM VEHICLE v "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "ORDER BY v.vehicleId LIMIT " + to_string(recordsPerPage) +
            " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        result = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License"
            << setw(15) << "Brand" << setw(15) << "Model"
            << setw(8) << "Year" << setw(25) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        while ((row = mysql_fetch_row(result))) {
            cout << left << setw(5) << row[0] << setw(15) << row[1]
                << setw(15) << row[2] << setw(15) << row[3]
                << setw(8) << row[4] << setw(25) << row[5] << endl;
        }
        mysql_free_result(result);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " vehicle(s)\033[0m" << endl;
        cout << "\n\033[36m[N]ext | [P]revious | [S]earch | [E]xit\033[0m" << endl;

        try {
            string input = getValidString("Enter choice", 1, 1, false);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'S') { searchVehicle(); return; }
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) { break; }
    }
}

// ============================================
// UPDATE VEHICLE
// ============================================
void updateVehicle() {
    clearScreen();
    printSectionTitle("UPDATE VEHICLE");

    try {
        string term = getValidString("Enter Plate or Owner Name");
        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, c.customerName "
            "FROM VEHICLE v JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE (v.licensePlate LIKE '%" + term + "%' OR c.customerName LIKE '%" + term + "%')";

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { showError("No match."); pause(); return; }

        cout << "\n\033[1;97m=== Matches ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(15) << "Brand" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(15) << row[1] << setw(15) << row[2] << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int id = getValidInt("\nEnter Vehicle ID", 1, 99999);

        cout << "\n1. Plate\n2. Brand\n3. Model\n4. Year\n5. Color\n6. Update All\n";
        int choice = getValidInt("Select Field", 1, 6);

        string updateQ = "UPDATE VEHICLE SET ";
        if (choice == 1 || choice == 6) updateQ += "licensePlate='" + getValidString("New Plate") + "',";
        if (choice == 2 || choice == 6) updateQ += "brand='" + getValidString("New Brand") + "',";
        if (choice == 3 || choice == 6) updateQ += "model='" + getValidString("New Model") + "',";
        if (choice == 4 || choice == 6) updateQ += "year='" + getValidYear("New Year") + "',";
        if (choice == 5 || choice == 6) updateQ += "color='" + getValidString("New Color") + "',";

        updateQ.pop_back();
        updateQ += " WHERE vehicleId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            showSuccess("Vehicle Updated Successfully!");

            query = "SELECT vehicleId, licensePlate, brand, model, color FROM VEHICLE WHERE vehicleId=" + to_string(id);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            cout << "\n\033[1;97m=== Updated Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(15) << "Brand" << setw(10) << "Color" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(15) << row[1] << setw(15) << row[2] << setw(10) << row[4] << endl;
            mysql_free_result(res);
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// VIEW VEHICLE SERVICE HISTORY
// ============================================
void viewVehicleServiceHistory() {
    clearScreen();
    printSectionTitle("VIEW VEHICLE SERVICE HISTORY");

    try {
        string term = getValidString("Enter License Plate or Owner Name");

        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, c.customerName "
            "FROM VEHICLE v "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE v.licensePlate LIKE '%" + term + "%' "
            "OR c.customerName LIKE '%" + term + "%'";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }

        MYSQL_RES* res = mysql_store_result(conn);
        int num_rows = mysql_num_rows(res);

        if (num_rows == 0) {
            showError("No vehicle found matching '" + term + "'");
            mysql_free_result(res);
            pause();
            return;
        }

        cout << "\n\033[1;97m=== Select Vehicle ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(20) << "Vehicle" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(15) << row[1]
                << setw(20) << (string(row[2]) + " " + row[3]) << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int vehicleId = getValidInt("\nEnter Vehicle ID to view history", 1, 99999);

        // Display History
        query = "SELECT a.appointmentId, st.slotDate, st.slotTime, "
            "GROUP_CONCAT(srv.serviceName SEPARATOR ', ') AS services, a.status "
            "FROM APPOINTMENT a "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "JOIN APPOINTMENT_SERVICE aps ON a.appointmentId = aps.appointmentId "
            "JOIN SERVICE_TYPE srv ON aps.serviceTypeId = srv.serviceTypeId "
            "WHERE a.vehicleId = " + to_string(vehicleId) + " "
            "GROUP BY a.appointmentId ORDER BY st.slotDate DESC";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); pause(); return; }

        res = mysql_store_result(conn);
        num_rows = mysql_num_rows(res);

        cout << "\n\033[1;97m=== Service History ===\033[0m" << endl;
        if (num_rows == 0) {
            showInfo("No service history records found for this vehicle.");
        }
        else {
            cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "Date" << setw(10) << "Time"
                << setw(45) << "Services" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            while ((row = mysql_fetch_row(res))) {
                string id = row[0];
                string date = row[1];
                string time = row[2];
                string rawServices = row[3] ? row[3] : u8"─";
                string status = row[4];

                int colWidth = 45;
                int textMax = 42;
                int len = rawServices.length();

                for (int i = 0; i < len; i += textMax) {
                    string chunk = rawServices.substr(i, textMax);

                    if (i == 0) {
                        cout << left << setw(5) << id
                            << setw(12) << date
                            << setw(10) << time
                            << setw(colWidth) << chunk
                            << setw(15) << status << endl;
                    }
                    else {
                        cout << left << setw(5) << " "
                            << setw(12) << " "
                            << setw(10) << " "
                            << setw(colWidth) << chunk
                            << setw(15) << " " << endl;
                    }
                }
            }
        }
        mysql_free_result(res);

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// TRACK VEHICLE RECORDS MENU
// ============================================
void trackVehicleRecords() {
    int choice;
    do {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[1;97m5.0 VEHICLE REGISTRY\033[0m\n" << endl;
        cout << "\033[36m1.\033[0m Search Vehicle" << endl;
        cout << "\033[36m2.\033[0m View Service History" << endl;
        cout << "\033[36m3.\033[0m Register New Vehicle" << endl;
        cout << "\033[36m4.\033[0m Update Vehicle Data" << endl;
        cout << "\033[36m5.\033[0m View Full Vehicle List" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 5);
            switch (choice) {
            case 1: searchVehicle(); break;
            case 2: viewVehicleServiceHistory(); break;
            case 3: addVehicle(); break;
            case 4: updateVehicle(); break;
            case 5: viewVehicles(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause(); }
    } while (choice != 0);
}
