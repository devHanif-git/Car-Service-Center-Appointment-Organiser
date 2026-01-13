#include "vehicle.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"


// ============================================
// HELPER DISPLAY FUNCTION
// ============================================
void displayCustomerVehicles(int customerId) {
    MYSQL_RES* result;
    MYSQL_ROW row;

    string query = "SELECT vehicleId, licensePlate, brand, model, color FROM VEHICLE "
        "WHERE customerId = " + to_string(customerId);

    if (mysql_query(conn, query.c_str())) {
        showError("Error fetching vehicles: " + string(mysql_error(conn)));
        return;
    }

    result = mysql_store_result(conn);
    int num_rows = mysql_num_rows(result);

    if (num_rows == 0) {
        showInfo("This customer has no registered vehicles.");
    }
    else {
        cout << "\n\033[1;97m=== Existing Vehicles for Customer ID " << customerId << " ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(15) << "Brand"
            << setw(15) << "Model" << setw(10) << "Color" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        while ((row = mysql_fetch_row(result))) {
            cout << left << setw(5) << row[0] << setw(15) << row[1] << setw(15) << row[2]
                << setw(15) << row[3] << setw(10) << row[4] << endl;
        }
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────" << "\033[0m" << endl;
    }
    mysql_free_result(result);
}

// ============================================
// ADD VEHICLE
// ============================================
void addVehicle() {
    clearScreen();
    printSectionTitle("ADD NEW VEHICLE");
    cout << endl;

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
        cout << "\033[36m" << left << setw(5) << "No." << setw(25) << "Name" << setw(15) << "Phone" << setw(25) << "Email" << "\033[0m" << endl;
        cout << "\033[90m" << u8"──────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        vector<int> customerIds;
        vector<string> customerNames;
        while ((row = mysql_fetch_row(result))) {
            customerIds.push_back(atoi(row[0]));
            customerNames.push_back(row[1]);
            cout << left << setw(5) << customerIds.size() << setw(25) << row[1] << setw(15) << row[2] << setw(25) << row[3] << endl;
        }
        mysql_free_result(result);

        int customerChoice = getValidInt("\nEnter Customer No.", 1, (int)customerIds.size());
        int customerId = customerIds[customerChoice - 1];
        string custName = customerNames[customerChoice - 1];

        displayCustomerVehicles(customerId);

        // Loop to add vehicles
        while (true) {
            clearScreen();
            printSectionTitle("ADD NEW VEHICLE");
            cout << endl;
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
                clearScreen();
                printSectionTitle("ADD NEW VEHICLE");
                showSuccess("Vehicle Added Successfully!");
                
                displayCustomerVehicles(customerId);
            }

            if (!getConfirmation("\nAdd another vehicle for this customer?")) {
                break;
            }
        }
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Vehicles");
}

// ============================================
// SEARCH VEHICLE
// ============================================
void searchVehicle() {
    clearScreen();
    printSectionTitle("VEHICLE SEARCH");
    cout << endl;

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
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Vehicles");
}

// ============================================
// VIEW VEHICLES
// ============================================
void viewVehicles() {
    clearScreen();
    printSectionTitle("VIEW ALL VEHICLES");
    cout << endl;

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

    // Redraw callback for paging
    auto redrawPage = [&]() {
        displayBreadcrumb();
        printSectionTitle("VIEW ALL VEHICLES");
        cout << endl;

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, v.year, c.customerName "
            "FROM VEHICLE v "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "ORDER BY v.vehicleId LIMIT " + to_string(recordsPerPage) +
            " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License"
            << setw(15) << "Brand" << setw(15) << "Model"
            << setw(8) << "Year" << setw(25) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW r;
        while ((r = mysql_fetch_row(res))) {
            cout << left << setw(5) << r[0] << setw(15) << r[1]
                << setw(15) << r[2] << setw(15) << r[3]
                << setw(8) << r[4] << setw(25) << r[5] << endl;
        }
        mysql_free_result(res);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " vehicle(s)\033[0m" << endl;
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
            else if (choice == 'S') { searchVehicle(); return; }
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) {  pause(); break; }
    }
    setBreadcrumb("Home > Vehicles");
}

// ============================================
// UPDATE VEHICLE
// ============================================
void updateVehicle() {
    clearScreen();
    printSectionTitle("UPDATE VEHICLE");
    cout << endl;

    try {
        string term = getValidString("Enter Plate or Owner Name");
        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, c.customerName "
            "FROM VEHICLE v JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE (v.licensePlate LIKE '%" + term + "%' OR c.customerName LIKE '%" + term + "%')";

        if (mysql_query(conn, query.c_str())) return;
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { showError("No match."); pause(); return; }

        cout << "\n\033[1;97m=== Matches ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "No." << setw(15) << "License" << setw(15) << "Brand" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        vector<int> vehicleIds;
        while ((row = mysql_fetch_row(res))) {
            vehicleIds.push_back(atoi(row[0]));
            cout << left << setw(5) << vehicleIds.size() << setw(15) << row[1] << setw(15) << row[2] << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int vehicleChoice = getValidInt("\nEnter Vehicle No.", 1, (int)vehicleIds.size());
        int id = vehicleIds[vehicleChoice - 1];

        // Fetch complete vehicle and owner details
        string detailQuery = "SELECT v.licensePlate, v.brand, v.model, v.year, v.color, "
            "c.customerName, c.phoneNumber, c.email "
            "FROM VEHICLE v JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE v.vehicleId = " + to_string(id);

        if (mysql_query(conn, detailQuery.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* detailRes = mysql_store_result(conn);
        MYSQL_ROW detailRow = mysql_fetch_row(detailRes);

        // Store current values
        string currPlate = detailRow[0];
        string currBrand = detailRow[1];
        string currModel = detailRow[2];
        string currYear = detailRow[3];
        string currColor = detailRow[4];

        clearScreen();
        printSectionTitle("UPDATE VEHICLE");
        cout << endl;

        // Display owner details
        cout << "\n\033[1;97m=== Current Owner Details ===\033[0m" << endl;
        cout << "\033[36mName:\033[0m  " << detailRow[5] << endl;
        cout << "\033[36mPhone:\033[0m " << detailRow[6] << endl;
        cout << "\033[36mEmail:\033[0m " << detailRow[7] << endl;

        // Display vehicle details
        cout << "\n\033[1;97m=== Vehicle Details ===\033[0m" << endl;
        cout << "\033[36mPlate:\033[0m " << currPlate << endl;
        cout << "\033[36mBrand:\033[0m " << currBrand << endl;
        cout << "\033[36mModel:\033[0m " << currModel << endl;
        cout << "\033[36mYear:\033[0m  " << currYear << endl;
        cout << "\033[36mColor:\033[0m " << currColor << endl;

        mysql_free_result(detailRes);

        cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;
        cout << "\n\033[1;97m=== Select Field to Update ===\033[0m" << endl;
        cout << "1. Plate\n2. Brand\n3. Model\n4. Year\n5. Color\n6. Update All\n";
        int choice = getValidInt("Select Field", 1, 6);

        cout << endl;

        string updateQ = "UPDATE VEHICLE SET ";
        string newVal;

        if (choice == 1 || choice == 6) {
            while (true) {
                newVal = getValidString("New Plate");
                if (newVal == currPlate) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "licensePlate='" + newVal + "',";
        }
        if (choice == 2 || choice == 6) {
            while (true) {
                newVal = getValidString("New Brand");
                if (newVal == currBrand) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "brand='" + newVal + "',";
        }
        if (choice == 3 || choice == 6) {
            while (true) {
                newVal = getValidString("New Model");
                if (newVal == currModel) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "model='" + newVal + "',";
        }
        if (choice == 4 || choice == 6) {
            while (true) {
                newVal = getValidYear("New Year");
                if (newVal == currYear) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "year='" + newVal + "',";
        }
        if (choice == 5 || choice == 6) {
            while (true) {
                newVal = getValidString("New Color");
                if (newVal == currColor) {
                    showWarning("New value cannot be the same as current value.");
                    continue;
                }
                break;
            }
            updateQ += "color='" + newVal + "',";
        }

        updateQ.pop_back();
        updateQ += " WHERE vehicleId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            clearScreen();
            printSectionTitle("UPDATE VEHICLE");
            
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
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Vehicles");
}

// ============================================
// VIEW VEHICLE SERVICE HISTORY
// ============================================
void viewVehicleServiceHistory() {
    clearScreen();
    printSectionTitle("VIEW VEHICLE SERVICE HISTORY");
    cout << endl;

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
        cout << "\033[36m" << left << setw(5) << "No." << setw(15) << "License" << setw(20) << "Vehicle" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        vector<int> historyVehicleIds;
        while ((row = mysql_fetch_row(res))) {
            historyVehicleIds.push_back(atoi(row[0]));
            cout << left << setw(5) << historyVehicleIds.size() << setw(15) << row[1]
                << setw(20) << (string(row[2]) + " " + row[3]) << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int vehicleChoice = getValidInt("\nEnter Vehicle No.", 1, (int)historyVehicleIds.size());
        int vehicleId = historyVehicleIds[vehicleChoice - 1];

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

        clearScreen();
        printSectionTitle("VIEW VEHICLE SERVICE HISTORY");
        cout << endl;

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
        pause();
    }
    catch (OperationCancelledException&) { pause(); }
    setBreadcrumb("Home > Vehicles");
}

// ============================================
// TRACK VEHICLE RECORDS MENU
// ============================================
void trackVehicleRecords() {
    int choice;

    auto displayMenu = [&]() {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[36m1.\033[0m Search Vehicle" << endl;
        cout << "\033[36m2.\033[0m View Service History" << endl;
        cout << "\033[36m3.\033[0m Register New Vehicle" << endl;
        cout << "\033[36m4.\033[0m Update Vehicle Data" << endl;
        cout << "\033[36m5.\033[0m View Full Vehicle List" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;
    };

    do {
        displayMenu();

        try {
            choice = getMenuChoice("\nEnter choice", 0, 5, displayMenu);
            switch (choice) {
            case 1: searchVehicle(); break;
            case 2: viewVehicleServiceHistory(); break;
            case 3: addVehicle(); break;
            case 4: updateVehicle(); break;
            case 5: viewVehicles(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause(); break; }
    } while (choice != 0);
}
