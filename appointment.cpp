#include "appointment.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// VIEW AVAILABLE SLOTS
// ============================================
void viewAvailableSlots() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("VIEW AVAILABLE SLOTS");

    try {
        printSubHeader("Search Availability");
        string startDate = getSmartDateInput("Enter Start Date (YYYYMMDD)", false);
        string endDate = getSmartDateInput("Enter End Date (YYYYMMDD)  ", false);

        if (startDate > endDate) {
            showWarning("Invalid Range: Start Date cannot be after End Date.");
            pause();
            return;
        }

        string selectClause = "SELECT st.slotDate, st.slotTimeId, st.slotTime, "
            "( "
            "   (SELECT COUNT(*) FROM SERVICE_BAY WHERE bayStatus != 'Maintenance') - "
            "   (SELECT COUNT(*) FROM APPOINTMENT a "
            "    JOIN SLOT_TIME st_start ON a.slotTimeId = st_start.slotTimeId "
            "    WHERE st_start.slotDate = st.slotDate "
            "    AND a.status IN ('Scheduled', 'In Progress') "
            "    AND st_start.slotTime <= st.slotTime "
            "    AND a.endTime > st.slotTime "
            "   ) "
            ") AS available_bays "
            "FROM SLOT_TIME st ";

        string whereClause = "WHERE st.slotDate BETWEEN '" + startDate + "' AND '" + endDate + "' "
            "AND (st.slotDate != CURDATE() OR st.slotTime > CURTIME()) "
            "HAVING available_bays > 0 ";

        string countQuery = "SELECT COUNT(*) FROM (" + selectClause + whereClause + ") AS temp_table";

        if (mysql_query(conn, countQuery.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        int totalRecords = atoi(row[0]);
        mysql_free_result(result);

        if (totalRecords == 0) {
            showWarning("Fully Booked or No Slots generated for this period.");
            pause();
            return;
        }

        int recordsPerPage = 19;
        int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;
        int currentPage = 1;

        while (true) {
            clearScreen();
            printSectionTitle("AVAILABLE SLOTS: " + startDate + " to " + endDate);

            int offset = (currentPage - 1) * recordsPerPage;

            string dataQuery = selectClause + whereClause +
                "ORDER BY st.slotDate ASC, st.slotTime ASC " +
                "LIMIT " + to_string(recordsPerPage) + " OFFSET " + to_string(offset);

            if (mysql_query(conn, dataQuery.c_str())) {
                showError("Error: " + string(mysql_error(conn)));
                break;
            }

            result = mysql_store_result(conn);

            string currentHeaderDate = "";

            int slotNum = 0;
            while ((row = mysql_fetch_row(result))) {
                string rowDate = row[0];

                if (rowDate != currentHeaderDate) {
                    cout << "\033[1;97m[ " << rowDate << " ]\033[0m" << endl;
                    cout << "\033[36m" << left << setw(6) << "No." << setw(12) << "Time" << setw(12) << "Free Bays" << "\033[0m" << endl;
                    cout << "\033[90m" << u8"──────────────────────────────" << "\033[0m" << endl;
                    currentHeaderDate = rowDate;
                    slotNum = 0;
                }

                slotNum++;
                cout << left << setw(6) << slotNum << setw(12) << row[2]
                    << setw(12) << row[3] << endl;
            }
            mysql_free_result(result);

            cout << u8"\n────────────────────────────────────────\033[0m" << endl;
            cout << "\033[90mPage " << currentPage << " of " << totalPages << " | Total Slots: " << totalRecords << "\033[0m" << endl;
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
    catch (OperationCancelledException&) {}
}

// ============================================
// CREATE APPOINTMENT
// ============================================
void createAppointment() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("CREATE APPOINTMENT");

    try {
        // STEP 1: SELECT VEHICLE
        printSubHeader("Select Vehicle");
        string term = getValidString("Enter Plate, Brand, or Owner Name");

        string query = "SELECT v.vehicleId, v.licensePlate, v.brand, v.model, c.customerName "
            "FROM VEHICLE v JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE (v.licensePlate LIKE '%" + term + "%' OR v.brand LIKE '%" + term + "%' OR c.customerName LIKE '%" + term + "%') ";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) {
            showError("No active vehicle found matching '" + term + "'");
            mysql_free_result(res); pause(); return;
        }

        printSectionDivider("Matching Vehicles", 55);
        cout << "\033[36m" << left << setw(5) << "No." << setw(15) << "License" << setw(20) << "Vehicle" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;

        vector<int> vehicleIds;
        while ((row = mysql_fetch_row(res))) {
            vehicleIds.push_back(atoi(row[0]));
            cout << left << setw(5) << vehicleIds.size() << setw(15) << row[1]
                << setw(20) << (string(row[2]) + " " + row[3]) << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int vehicleChoice = getValidInt("\nEnter Vehicle No.", 1, (int)vehicleIds.size());
        int vehicleId = vehicleIds[vehicleChoice - 1];

        // STEP 2: SELECT SERVICES
        cout << endl;
        printSectionDivider("Available Services", 45);
        query = "SELECT serviceTypeId, serviceName, standardDuration, basePrice FROM SERVICE_TYPE ORDER BY serviceName";
        mysql_query(conn, query.c_str());
        res = mysql_store_result(conn);

        vector<int> serviceTypeIds;
        vector<int> serviceDurations;
        vector<string> serviceNames;

        cout << "\033[36m" << left << setw(5) << "No." << setw(25) << "Service" << setw(10) << "Mins" << setw(10) << "Price" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────" << "\033[0m" << endl;
        while ((row = mysql_fetch_row(res))) {
            serviceTypeIds.push_back(atoi(row[0]));
            serviceNames.push_back(row[1]);
            serviceDurations.push_back(atoi(row[2]));
            cout << left << setw(5) << serviceTypeIds.size() << setw(25) << row[1] << setw(10) << row[2] << setw(10) << row[3] << endl;
        }
        mysql_free_result(res);

        cout << "\033[90m" << u8"────────────────────────────────────────────────────" << "\033[0m" << endl;
        cout << "\033[36m0.\033[0m  Done selecting\n" << endl;

        vector<int> serviceIds;
        vector<string> selectedNames;
        int totalDuration = 0;

        while (true) {
            int serviceChoice = getValidInt("Enter Service No. (or 0 when done)", 0, (int)serviceTypeIds.size());

            if (serviceChoice == 0) {
                if (serviceIds.empty()) {
                    showWarning("Please select at least one service.");
                    continue;
                }
                break;
            }

            int selectedId = serviceTypeIds[serviceChoice - 1];
            string selectedName = serviceNames[serviceChoice - 1];

            // Check for duplicate
            bool alreadySelected = false;
            for (int id : serviceIds) {
                if (id == selectedId) {
                    alreadySelected = true;
                    break;
                }
            }

            if (alreadySelected) {
                showWarning("'" + selectedName + "' is already selected.");
                continue;
            }

            serviceIds.push_back(selectedId);
            selectedNames.push_back(selectedName);
            totalDuration += serviceDurations[serviceChoice - 1];

            showSuccess("Added: " + selectedName);

            // Show selected services so far
            cout << "\033[90mSelected: ";
            for (size_t i = 0; i < selectedNames.size(); i++) {
                cout << selectedNames[i];
                if (i < selectedNames.size() - 1) {
                    cout << ", ";
                    if ((i + 1) % 3 == 0) cout << "\n          ";  // newline after every 3
                }
            }
            cout << "\033[0m\n" << endl;
        }

        // STEP 3: DATE & TIME
        cout << endl;
        printSubHeader("Schedule");

        // Show total duration and inform about slot filtering
        cout << "\033[90mTotal service time: " << totalDuration << " mins. ";
        if (totalDuration > 45) {
            cout << "Late slots will be limited to ensure completion by 17:15.\033[0m" << endl;
        } else {
            cout << "\033[0m" << endl;
        }

        string rawDate = getSmartDateInput("Enter Date (YYYYMMDD)", false);
        string date = "'" + rawDate + "'";

        // Check if vehicle already has an active appointment on this date
        string dupCheck = "SELECT a.appointmentId, st.slotTime, a.status, sb.bayName "
            "FROM APPOINTMENT a "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "JOIN SERVICE_BAY sb ON a.serviceBayId = sb.serviceBayId "
            "WHERE a.vehicleId = " + to_string(vehicleId) + " "
            "AND st.slotDate = " + date + " "
            "AND a.status IN ('Scheduled', 'In Progress')";

        if (mysql_query(conn, dupCheck.c_str()) == 0) {
            MYSQL_RES* dupRes = mysql_store_result(conn);
            if (mysql_num_rows(dupRes) > 0) {
                MYSQL_ROW dupRow = mysql_fetch_row(dupRes);
                showError("This vehicle already has an active appointment on " + rawDate + "!");
                showWarning("Existing: Appointment #" + string(dupRow[0]) + " at " + dupRow[1] + " (" + dupRow[3] + ") - Status: " + dupRow[2]);
                showInfo("Please complete or cancel the existing appointment first.");
                mysql_free_result(dupRes);
                pause();
                return;
            }
            mysql_free_result(dupRes);
        }

        // Dynamic availability: Count non-maintenance bays minus overlapping appointments
        // Also filter slots where service would extend past 17:15 (15-min buffer after closing)
        query = "SELECT st.slotTimeId, st.slotTime, "
            "( "
            "   (SELECT COUNT(*) FROM SERVICE_BAY WHERE bayStatus != 'Maintenance') - "
            "   (SELECT COUNT(*) FROM APPOINTMENT a "
            "    JOIN SLOT_TIME st_start ON a.slotTimeId = st_start.slotTimeId "
            "    WHERE st_start.slotDate = " + date + " "
            "    AND a.status IN ('Scheduled', 'In Progress') "
            "    AND st_start.slotTime <= st.slotTime "
            "    AND a.endTime > st.slotTime "
            "   ) "
            ") AS avail "
            "FROM SLOT_TIME st "
            "WHERE st.slotDate = " + date + " "
            "AND (st.slotDate != CURDATE() OR st.slotTime > CURTIME()) "
            "AND ADDTIME(st.slotTime, SEC_TO_TIME(" + to_string(totalDuration * 60) + ")) <= '17:15:00' "
            "HAVING avail > 0 "
            "ORDER BY st.slotTime";

        if (mysql_query(conn, query.c_str())) { showError("Invalid Date."); return; }
        res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) {
            showError("No slots available for " + rawDate + ".");
            showInfo("Possible reasons: Fully booked, past date, or service duration (" +
                to_string(totalDuration) + " mins) exceeds remaining time before closing.");
            mysql_free_result(res); pause(); return;
        }

        int validSlots[50];
        int slotCount = 0;

        cout << "\n\033[1;97m=== Available Slots for " << rawDate << " ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(6) << "No." << setw(12) << "Time" << setw(10) << "Left" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────" << "\033[0m" << endl;
        while ((row = mysql_fetch_row(res))) {
            validSlots[slotCount] = atoi(row[0]);
            slotCount++;
            cout << left << setw(6) << slotCount << setw(12) << row[1] << setw(10) << row[2] << endl;
        }
        mysql_free_result(res);

        int slotTimeId;
        while (true) {
            int slotChoice = getValidInt("\nEnter Slot No.", 1, slotCount);

            if (slotChoice >= 1 && slotChoice <= slotCount) {
                slotTimeId = validSlots[slotChoice - 1];
                break;
            }
            showError("Invalid selection. Please choose from 1 to " + to_string(slotCount) + ".");
        }

        query = "SELECT slotTime FROM SLOT_TIME WHERE slotTimeId = " + to_string(slotTimeId);
        mysql_query(conn, query.c_str());
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        string startTime = row[0];
        mysql_free_result(res);

        query = "SELECT ADDTIME('" + startTime + "', SEC_TO_TIME(" + to_string(totalDuration * 60) + "))";
        mysql_query(conn, query.c_str());
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        string endTime = row[0];
        mysql_free_result(res);

        // Safety check: Ensure service completes by 17:15 (15-min buffer after closing)
        if (endTime > "17:15:00") {
            showError("Selected slot would extend past closing time (17:15).");
            showInfo("Total service duration: " + to_string(totalDuration) + " mins. Please choose an earlier slot.");
            pause();
            return;
        }

        // STEP 4: BAY ALLOCATION
        query = "SELECT serviceBayId FROM SERVICE_BAY sb "
            "WHERE bayStatus = 'Available' "
            "AND NOT EXISTS ("
            "SELECT 1 FROM APPOINTMENT a "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE a.serviceBayId = sb.serviceBayId "
            "AND st.slotDate = " + date + " "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "AND st.slotTime < '" + endTime + "' "
            "AND a.endTime > '" + startTime + "'"
            ") LIMIT 1";

        if (mysql_query(conn, query.c_str())) { cout << mysql_error(conn); return; }
        res = mysql_store_result(conn);

        int bayId = 0;
        if (mysql_num_rows(res) > 0) {
            row = mysql_fetch_row(res);
            bayId = atoi(row[0]);
        }
        mysql_free_result(res);

        if (bayId == 0) {
            showError("System Alert: No Service Bays are available for " + to_string(totalDuration) + " mins at " + startTime + ".");
            showError("Please choose a different time slot or date.");
            pause();
            return;
        }

        // STEP 5: FINALIZE
        string notes = getValidString("Enter Notes (Optional)", 0, 100, true);

        query = "INSERT INTO APPOINTMENT (vehicleId, serviceBayId, slotTimeId, endTime, status, notes, createdDate) "
            "VALUES (" + to_string(vehicleId) + ", " + to_string(bayId) + ", " + to_string(slotTimeId) +
            ", '" + endTime + "', 'Scheduled', '" + notes + "', NOW())";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            int appId = mysql_insert_id(conn);

            for (size_t i = 0; i < serviceIds.size(); i++) {
                string sQuery = "INSERT INTO APPOINTMENT_SERVICE (appointmentId, serviceTypeId, serviceStatus) VALUES ("
                    + to_string(appId) + ", " + to_string(serviceIds[i]) + ", 'Pending')";
                mysql_query(conn, sQuery.c_str());
            }

            mysql_query(conn, ("UPDATE SLOT_TIME SET currentBookings = currentBookings + 1 WHERE slotTimeId = " + to_string(slotTimeId)).c_str());

            // Get bay name for display
            string bayNameQuery = "SELECT bayName FROM SERVICE_BAY WHERE serviceBayId = " + to_string(bayId);
            mysql_query(conn, bayNameQuery.c_str());
            MYSQL_RES* bayRes = mysql_store_result(conn);
            MYSQL_ROW bayRow = mysql_fetch_row(bayRes);
            string bayName = bayRow[0];
            mysql_free_result(bayRes);

            showSuccess("Appointment Created Successfully! (ID: " + to_string(appId) + ")");
            showSuccess("Assigned to: " + bayName);
            showSuccess("Estimated End Time: " + endTime);
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// VIEW APPOINTMENTS
// ============================================
void viewAppointments() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("VIEW APPOINTMENTS");

    try {
        printSubHeader("Filter Options");
        cout << "\033[36m1.\033[0m Search by Keyword (Name or Plate)" << endl;
        cout << "\033[36m2.\033[0m Filter by Date" << endl;
        cout << "\033[36m3.\033[0m Filter by Status" << endl;
        cout << "\033[36m4.\033[0m View All" << endl;

        int choice = getValidInt("\nEnter choice", 1, 4);

        string whereClause = "";

        switch (choice) {
        case 1: {
            string term = getValidString("Enter Customer Name or License Plate");
            whereClause = "WHERE (c.customerName LIKE '%" + term + "%' OR v.licensePlate LIKE '%" + term + "%') ";
            break;
        }
        case 2: {
            string date = getSmartDateInput("Enter Date (YYYYMMDD)");
            whereClause = "WHERE st.slotDate = '" + date + "' ";
            break;
        }
        case 3: {
            cout << "\n1. Scheduled\n2. In Progress\n3. Completed\n4. Cancelled\n";
            int s = getValidInt("Select Status", 1, 4);
            string statuses[] = { "", "Scheduled", "In Progress", "Completed", "Cancelled" };
            whereClause = "WHERE a.status = '" + statuses[s] + "' ";
            break;
        }
        case 4:
            whereClause = "";
            break;
        }

        string countQuery = "SELECT COUNT(*) FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SERVICE_BAY sb ON a.serviceBayId = sb.serviceBayId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId " + whereClause;

        if (mysql_query(conn, countQuery.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);
        int totalRecords = atoi(row[0]);
        mysql_free_result(result);

        if (totalRecords == 0) {
            showError("No appointments found matching your criteria.");
            pause();
            return;
        }

        int recordsPerPage = 10;
        int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;
        int currentPage = 1;

        while (true) {
            clearScreen();
            printSectionTitle("VIEW APPOINTMENTS - Page " + to_string(currentPage) + "/" + to_string(totalPages));

            int offset = (currentPage - 1) * recordsPerPage;

            string dataQuery = "SELECT a.appointmentId, v.licensePlate, c.customerName, sb.bayName, "
                "st.slotDate, st.slotTime, a.status, "
                "GROUP_CONCAT(DISTINCT s.fullName SEPARATOR ', ') as mechanics "
                "FROM APPOINTMENT a "
                "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
                "JOIN CUSTOMER c ON v.customerId = c.customerId "
                "JOIN SERVICE_BAY sb ON a.serviceBayId = sb.serviceBayId "
                "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
                "LEFT JOIN APPOINTMENT_SERVICE aps ON a.appointmentId = aps.appointmentId "
                "LEFT JOIN STAFF s ON aps.staffId = s.staffId "
                + whereClause +
                " GROUP BY a.appointmentId "
                " ORDER BY st.slotDate DESC, st.slotTime ASC "
                "LIMIT " + to_string(recordsPerPage) + " OFFSET " + to_string(offset);

            if (mysql_query(conn, dataQuery.c_str())) {
                showError("Error: " + string(mysql_error(conn)));
                break;
            }

            result = mysql_store_result(conn);

            cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "License" << setw(20) << "Customer"
                << setw(8) << "Bay" << setw(12) << "Date" << setw(10) << "Start"
                << setw(20) << "Mechanic" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            while ((row = mysql_fetch_row(result))) {
                string mechName = (row[7] ? row[7] : "-");
                string status = row[6];
                string colorStatus = status;

                if (status == "In Progress") colorStatus = "\033[33mIn Progress\033[0m";
                else if (status == "Completed") colorStatus = "\033[32mCompleted\033[0m";
                else if (status == "Cancelled") colorStatus = "\033[31mCancelled\033[0m";
                else if (status == "Scheduled") colorStatus = "\033[36mScheduled\033[0m";

                int colWidth = 20;
                int textMax = 18;
                int len = mechName.length();

                if (len == 0) {
                    cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(20) << row[2]
                        << setw(8) << row[3] << setw(12) << row[4] << setw(10) << row[5]
                        << setw(colWidth) << "-" << setw(25) << colorStatus << endl;
                }
                else {
                    for (int i = 0; i < len; i += textMax) {
                        string chunk = mechName.substr(i, textMax);

                        if (i == 0) {
                            cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(20) << row[2]
                                << setw(8) << row[3] << setw(12) << row[4] << setw(10) << row[5]
                                << setw(colWidth) << chunk << setw(25) << colorStatus << endl;
                        }
                        else {
                            cout << left << setw(5) << " " << setw(12) << " " << setw(20) << " "
                                << setw(8) << " " << setw(12) << " " << setw(10) << " "
                                << setw(colWidth) << chunk << setw(25) << " \033[0m" << endl;
                        }
                    }
                }
            }
            mysql_free_result(result);

            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            cout << "\033[90mPage " << currentPage << " of " << totalPages << " | Total Records: " << totalRecords << "\033[0m" << endl;
            cout << "\n\033[36m[N]ext | [P]revious | [E]xit\033[0m" << endl;

            try {
                string input = getValidString("Enter choice", 1, 1, false);
                char nav = toupper(input[0]);

                if (nav == 'N' && currentPage < totalPages) currentPage++;
                else if (nav == 'P' && currentPage > 1) currentPage--;
                else if (nav == 'E') break;
            }
            catch (OperationCancelledException&) { break; }
        }

    }
    catch (OperationCancelledException&) {}
}

// ============================================
// UPDATE APPOINTMENT STATUS
// ============================================
void updateAppointmentStatus() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("UPDATE APPOINTMENT STATUS");

    try {
        cout << "\033[1;97mSearch Criteria:\033[0m" << endl;
        cout << "\033[90m- Leave empty and press Enter to see TODAY'S appointments.\033[0m" << endl;
        cout << "\033[90m- OR type a Customer Name / License Plate.\033[0m" << endl;

        string term = getValidString("Enter Search Term", 0, 50, true);

        string query = "SELECT a.appointmentId, st.slotDate, st.slotTime, c.customerName, v.licensePlate, a.status "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId ";

        if (term.empty()) {
            query += "WHERE st.slotDate = CURDATE() ";
        }
        else {
            query += "WHERE (c.customerName LIKE '%" + term + "%' OR v.licensePlate LIKE '%" + term + "%') ";
        }

        query += "ORDER BY st.slotDate DESC, st.slotTime ASC";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }

        MYSQL_RES* res = mysql_store_result(conn);
        int num_rows = mysql_num_rows(res);

        if (num_rows == 0) {
            showError("No appointments found matching your criteria.");
            mysql_free_result(res);
            pause();
            return;
        }

        cout << endl;
        printSectionDivider("Select Appointment", 72);
        cout << "\033[36m" << left << setw(5) << "No." << setw(12) << "Date" << setw(10) << "Time"
            << setw(20) << "Customer" << setw(12) << "Plate" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        vector<int> appointmentIds;
        while ((row = mysql_fetch_row(res))) {
            appointmentIds.push_back(atoi(row[0]));
            string status = row[5];
            string statusColor = "\033[0m";
            if (status == "Scheduled") statusColor = "\033[36m";       // Cyan/Blue
            else if (status == "In Progress") statusColor = "\033[33m"; // Yellow
            else if (status == "Completed") statusColor = "\033[32m";   // Green
            else if (status == "Cancelled") statusColor = "\033[31m";   // Red

            cout << left << setw(5) << appointmentIds.size() << setw(12) << row[1] << setw(10) << row[2]
                << setw(20) << row[3] << setw(12) << row[4] << statusColor << status << "\033[0m" << endl;
        }
        mysql_free_result(res);

        int appointmentChoice = getValidInt("\nEnter Appointment No.", 1, (int)appointmentIds.size());
        int appointmentId = appointmentIds[appointmentChoice - 1];

        cout << endl;
        cout << "\033[1;97m=== Set New Status ===\033[0m" << endl;
        cout << "\033[36m1.\033[0m Scheduled" << endl;
        cout << "\033[36m2.\033[0m In Progress" << endl;
        cout << "\033[36m3.\033[0m Completed" << endl;
        cout << "\033[36m4.\033[0m Cancelled" << endl;

        int statusChoice = getValidInt("Enter choice", 1, 4);
        string status[] = { "", "Scheduled", "In Progress", "Completed", "Cancelled" };

        // Get the serviceBayId for this appointment
        query = "SELECT serviceBayId FROM APPOINTMENT WHERE appointmentId = " + to_string(appointmentId);
        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* bayRes = mysql_store_result(conn);
        MYSQL_ROW bayRow = mysql_fetch_row(bayRes);
        int serviceBayId = atoi(bayRow[0]);
        mysql_free_result(bayRes);

        query = "UPDATE APPOINTMENT SET status = '" + status[statusChoice] + "' WHERE appointmentId = " + to_string(appointmentId);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            // Auto-update bay status based on appointment status
            string bayStatus = "";
            if (status[statusChoice] == "In Progress") {
                bayStatus = "Occupied";
            }
            else if (status[statusChoice] == "Completed" || status[statusChoice] == "Cancelled") {
                bayStatus = "Available";
            }

            if (!bayStatus.empty()) {
                string bayQuery = "UPDATE SERVICE_BAY SET bayStatus = '" + bayStatus + "' WHERE serviceBayId = " + to_string(serviceBayId);
                mysql_query(conn, bayQuery.c_str());
            }

            showSuccess("Status Updated Successfully to: " + status[statusChoice]);
            if (!bayStatus.empty()) {
                showInfo("Bay status auto-updated to: " + bayStatus);
            }
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// MANAGE SERVICE JOB DETAILS
// ============================================
void manageServiceJobDetails() {
    clearScreen();
    printSectionTitle(currentStaffName + "'S TASK MANAGER");

    try {
        vector<int> allJobIds;
        vector<int> allBayIds;
        vector<string> allSlotTimes;
        MYSQL_ROW row;

        // PART A: MY ASSIGNED JOBS (Appointments assigned to me)
        string myJobQuery = "SELECT a.appointmentId, v.licensePlate, c.customerName, sb.bayName, "
            "TIME_FORMAT(st.slotTime, '%H:%i') as startTime, a.status, a.serviceBayId, st.slotTime "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SERVICE_BAY sb ON a.serviceBayId = sb.serviceBayId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE a.assignedStaffId = " + to_string(currentStaffId) + " "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "AND st.slotDate = CURDATE() "
            "ORDER BY st.slotTime ASC";

        if (mysql_query(conn, myJobQuery.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* myRes = mysql_store_result(conn);
        int myJobCount = mysql_num_rows(myRes);

        if (myJobCount > 0) {
            printSectionDivider("MY ASSIGNED JOBS (Today)", 75, "\033[1;32m");
            cout << "\033[36m" << left << setw(5) << "No." << setw(12) << "Plate" << setw(16) << "Customer"
                << setw(10) << "Bay" << setw(10) << "Time" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            while ((row = mysql_fetch_row(myRes))) {
                allJobIds.push_back(atoi(row[0]));
                allBayIds.push_back(atoi(row[6]));
                allSlotTimes.push_back(row[7]);
                cout << "\033[32m" << left << setw(5) << allJobIds.size() << setw(12) << row[1] << setw(16) << row[2]
                    << setw(10) << row[3] << setw(10) << row[4] << setw(15) << row[5] << "\033[0m" << endl;
            }
        }
        else {
            showInfo("You have no assigned jobs right now.");
        }
        mysql_free_result(myRes);

        // PART B: JOB POOL (Only unassigned jobs)
        cout << endl;
        printSectionDivider("AVAILABLE JOB POOL (Today)", 75);

        string poolQuery = "SELECT a.appointmentId, v.licensePlate, c.customerName, sb.bayName, "
            "TIME_FORMAT(st.slotTime, '%H:%i') as startTime, a.status, a.serviceBayId, st.slotTime "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SERVICE_BAY sb ON a.serviceBayId = sb.serviceBayId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE st.slotDate = CURDATE() "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "AND a.assignedStaffId IS NULL "
            "ORDER BY st.slotTime ASC";

        if (mysql_query(conn, poolQuery.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* poolRes = mysql_store_result(conn);
        int poolCount = mysql_num_rows(poolRes);

        if (poolCount == 0) {
            showInfo("No unassigned jobs in pool.");
        }
        else {
            cout << "\033[36m" << left << setw(5) << "No." << setw(12) << "Plate" << setw(16) << "Customer"
                << setw(10) << "Bay" << setw(10) << "Time" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            while ((row = mysql_fetch_row(poolRes))) {
                allJobIds.push_back(atoi(row[0]));
                allBayIds.push_back(atoi(row[6]));
                allSlotTimes.push_back(row[7]);
                cout << left << setw(5) << allJobIds.size() << setw(12) << row[1] << setw(16) << row[2]
                    << setw(10) << row[3] << setw(10) << row[4] << setw(15) << row[5] << endl;
            }
        }
        mysql_free_result(poolRes);

        // Check if there are any jobs at all
        if (allJobIds.empty()) {
            // showWarning("No jobs available today.");
            pause();
            return;
        }

        // SELECT JOB
        cout << endl;
        if (myJobCount > 0 && poolCount > 0) {
            showInfo("Enter No. to work on your job or claim a new one (0 to Exit)");
        }
        else if (myJobCount > 0) {
            showInfo("Enter No. to continue working on your job (0 to Exit)");
        }
        else {
            showInfo("Enter No. to claim and work on a job (0 to Exit)");
        }

        int jobChoice = getValidInt("Enter No.", 0, (int)allJobIds.size());

        if (jobChoice == 0) return;
        int appointmentId = allJobIds[jobChoice - 1];
        int selectedBayId = allBayIds[jobChoice - 1];
        string selectedTime = allSlotTimes[jobChoice - 1];

        // ENFORCE ORDER: Check if there's an earlier unassigned job on the same bay
        string orderCheck = "SELECT a.appointmentId, v.licensePlate, TIME_FORMAT(st.slotTime, '%H:%i') as startTime "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE a.serviceBayId = " + to_string(selectedBayId) + " "
            "AND st.slotDate = CURDATE() "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "AND a.assignedStaffId IS NULL "
            "AND st.slotTime < '" + selectedTime + "' "
            "AND a.appointmentId != " + to_string(appointmentId) + " "
            "ORDER BY st.slotTime ASC LIMIT 1";

        if (mysql_query(conn, orderCheck.c_str()) == 0) {
            MYSQL_RES* orderRes = mysql_store_result(conn);
            if (mysql_num_rows(orderRes) > 0) {
                MYSQL_ROW orderRow = mysql_fetch_row(orderRes);
                showError("Cannot take this job yet!");
                showWarning("Earlier job must be completed first on this bay:");
                showWarning("  -> " + string(orderRow[1]) + " at " + string(orderRow[2]) + " (Appointment #" + string(orderRow[0]) + ")");
                showInfo("Please select the earlier job first, or wait for it to be assigned.");
                mysql_free_result(orderRes);
                pause();
                return;
            }
            mysql_free_result(orderRes);
        }

        // TASK MANAGEMENT LOOP
        while (true) {
            clearScreen();
            printSectionTitle("MANAGING APPOINTMENT ID: " + to_string(appointmentId));

            string query = "SELECT aps.appointmentServiceId, st.serviceName, st.standardDuration, aps.actualDuration, aps.serviceStatus, aps.startedAt "
                "FROM APPOINTMENT_SERVICE aps "
                "JOIN SERVICE_TYPE st ON aps.serviceTypeId = st.serviceTypeId "
                "WHERE aps.appointmentId = " + to_string(appointmentId);

            if (mysql_query(conn, query.c_str())) break;
            MYSQL_RES* res = mysql_store_result(conn);

            printSubHeader("Service Task Checklist");
            cout << "\033[36m" << left << setw(5) << "No." << setw(25) << "Service Task" << setw(12) << "Est."
                << setw(12) << "Actual" << setw(15) << "Status" << setw(20) << "Started At" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            MYSQL_ROW row;
            vector<int> taskIds;
            while ((row = mysql_fetch_row(res))) {
                taskIds.push_back(atoi(row[0]));
                string actual = (row[3] ? row[3] : "-");
                string startInfo = (row[5] ? row[5] : "-");

                cout << left << setw(5) << taskIds.size() << setw(25) << row[1]
                    << setw(12) << (string(row[2]) + "m")
                    << setw(12) << (actual == "-" ? "-" : actual + "m")
                    << setw(15) << row[4]
                    << setw(20) << startInfo << endl;
            }
            mysql_free_result(res);

            showInfo("Enter Task No. to update status");
            int taskChoice = getValidInt("Enter No. (or 0 to Finish/Back)", 0, (int)taskIds.size());

            if (taskChoice == 0) break;
            int serviceId = taskIds[taskChoice - 1];

            cout << "\n1. Pending\n2. In Progress (Start Timer)\n3. Completed (Stop Timer)\n";
            int statusChoice = getValidInt("Select New Status", 1, 3);

            string updateQ = "";

            if (statusChoice == 2) {
                // Check if appointment is already assigned
                string checkAssign = "SELECT assignedStaffId FROM APPOINTMENT WHERE appointmentId = " + to_string(appointmentId);
                mysql_query(conn, checkAssign.c_str());
                MYSQL_RES* assignRes = mysql_store_result(conn);
                MYSQL_ROW assignRow = mysql_fetch_row(assignRes);
                int currentAssigned = (assignRow[0] ? atoi(assignRow[0]) : 0);
                mysql_free_result(assignRes);

                // If not assigned, claim the entire appointment
                if (currentAssigned == 0) {
                    // Assign appointment to this mechanic
                    string assignApp = "UPDATE APPOINTMENT SET assignedStaffId = " + to_string(currentStaffId) +
                        " WHERE appointmentId = " + to_string(appointmentId);
                    mysql_query(conn, assignApp.c_str());

                    // Assign ALL tasks to this mechanic
                    string assignTasks = "UPDATE APPOINTMENT_SERVICE SET staffId = " + to_string(currentStaffId) +
                        " WHERE appointmentId = " + to_string(appointmentId);
                    mysql_query(conn, assignTasks.c_str());

                    showSuccess("You are now assigned to this job. Complete ALL tasks.");
                }

                updateQ = "UPDATE APPOINTMENT_SERVICE SET "
                    "serviceStatus = 'In Progress', "
                    "startedAt = NOW(), "
                    "staffId = " + to_string(currentStaffId) + " "
                    "WHERE appointmentServiceId = " + to_string(serviceId);
            }
            else if (statusChoice == 3) {
                string checkStart = "SELECT startedAt FROM APPOINTMENT_SERVICE WHERE appointmentServiceId = " + to_string(serviceId);
                mysql_query(conn, checkStart.c_str());
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);
                string startedAt = (row[0] ? row[0] : "");
                mysql_free_result(res);

                if (!startedAt.empty()) {
                    updateQ = "UPDATE APPOINTMENT_SERVICE SET serviceStatus = 'Completed', "
                        "actualDuration = GREATEST(TIMESTAMPDIFF(MINUTE, startedAt, NOW()), 1) "
                        "WHERE appointmentServiceId = " + to_string(serviceId);
                    showSuccess("Auto-calculating duration...");
                }
                else {
                    showWarning("Task was not marked 'In Progress' earlier.");
                    int manualDuration = getValidInt("Enter Actual Duration (minutes)", 1, 480);
                    updateQ = "UPDATE APPOINTMENT_SERVICE SET serviceStatus = 'Completed', "
                        "actualDuration = " + to_string(manualDuration) + ", startedAt = NOW(), "
                        "staffId = " + to_string(currentStaffId) + " "
                        "WHERE appointmentServiceId = " + to_string(serviceId);
                }
            }
            else {
                updateQ = "UPDATE APPOINTMENT_SERVICE SET serviceStatus = 'Pending', startedAt = NULL, actualDuration = NULL, staffId = NULL "
                    "WHERE appointmentServiceId = " + to_string(serviceId);
            }

            if (mysql_query(conn, updateQ.c_str())) {
                showError("Update failed: " + string(mysql_error(conn)));
                pause();
            }
            else {
                string checkQ = "SELECT COUNT(*) FROM APPOINTMENT_SERVICE WHERE appointmentId = " + to_string(appointmentId) + " AND serviceStatus != 'Completed'";
                mysql_query(conn, checkQ.c_str());
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);
                int remainingTasks = atoi(row[0]);
                mysql_free_result(res);

                if (remainingTasks == 0) {
                    showWarning("SYSTEM ALERT: All tasks for this appointment are COMPLETED.");
                    if (getConfirmation("Mark Main Appointment as 'Completed' (Release Bay)?")) {
                        mysql_query(conn, ("UPDATE APPOINTMENT SET status = 'Completed' WHERE appointmentId = " + to_string(appointmentId)).c_str());

                        // Auto-release bay: Set bay status to Available
                        string bayQuery = "UPDATE SERVICE_BAY sb "
                            "JOIN APPOINTMENT a ON sb.serviceBayId = a.serviceBayId "
                            "SET sb.bayStatus = 'Available' "
                            "WHERE a.appointmentId = " + to_string(appointmentId);
                        mysql_query(conn, bayQuery.c_str());

                        showSuccess("Job Done. Bay released and available.");
                        pause();
                        break;
                    }
                }
                else {
                    if (statusChoice == 2) {
                        // Auto-occupy bay when work starts
                        string updateApp = "UPDATE APPOINTMENT SET status = 'In Progress' WHERE appointmentId = " + to_string(appointmentId) + " AND status = 'Scheduled'";
                        if (mysql_query(conn, updateApp.c_str()) == 0 && mysql_affected_rows(conn) > 0) {
                            string bayQuery = "UPDATE SERVICE_BAY sb "
                                "JOIN APPOINTMENT a ON sb.serviceBayId = a.serviceBayId "
                                "SET sb.bayStatus = 'Occupied' "
                                "WHERE a.appointmentId = " + to_string(appointmentId);
                            mysql_query(conn, bayQuery.c_str());
                        }
                    }
                }
            }
        }
    }
    catch (OperationCancelledException&) {}
}

// ============================================
// CANCEL APPOINTMENT
// ============================================
void cancelAppointment() {
    clearScreen();
    displayBreadcrumb();
    printSectionTitle("CANCEL APPOINTMENT");

    try {
        cout << "\033[1;97mSearch Appointment to Cancel:\033[0m" << endl;
        string term = getValidString("Enter Customer Name, Plate, or press Enter to list all active", 0, 50, true);

        string query = "SELECT a.appointmentId, st.slotDate, st.slotTime, c.customerName, v.licensePlate, a.status "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE a.status IN ('Scheduled', 'In Progress') ";

        if (!term.empty()) {
            query += "AND (c.customerName LIKE '%" + term + "%' OR v.licensePlate LIKE '%" + term + "%') ";
        }

        query += "ORDER BY st.slotDate, st.slotTime";

        if (mysql_query(conn, query.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }

        MYSQL_RES* res = mysql_store_result(conn);
        int num_rows = mysql_num_rows(res);

        if (num_rows == 0) {
            showError("No active appointments found.");
            mysql_free_result(res);
            pause();
            return;
        }

        printSectionDivider("Active Appointments", 72);
        cout << "\033[36m" << left << setw(5) << "No." << setw(12) << "Date" << setw(10) << "Time"
            << setw(20) << "Customer" << setw(12) << "Plate" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        vector<int> cancelIds;
        while ((row = mysql_fetch_row(res))) {
            cancelIds.push_back(atoi(row[0]));
            string status = row[5];
            string statusColor = "\033[0m";
            if (status == "Scheduled") statusColor = "\033[36m";       //Cyan/Blue
            if (status == "In Progress") statusColor = "\033[33m";     //Yellow
            if (status == "Completed") statusColor = "\033[32m";     //Green
            if (status == "Cancelled") statusColor = "\033[31m";     //Red

            
            cout << left << setw(5) << cancelIds.size() << setw(12) << row[1] << setw(10) << row[2]
                << setw(20) << row[3] << setw(12) << row[4] << statusColor << status << "\033[0m" << endl;
        }
        mysql_free_result(res);

        int cancelChoice = getValidInt("\nEnter Appointment No. to cancel", 1, (int)cancelIds.size());
        int appointmentId = cancelIds[cancelChoice - 1];

        query = "SELECT slotTimeId, serviceBayId FROM APPOINTMENT WHERE appointmentId = " + to_string(appointmentId);
        if (mysql_query(conn, query.c_str())) return;
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        int slotTimeId = atoi(row[0]);
        int serviceBayId = atoi(row[1]);
        mysql_free_result(res);

        if (getConfirmation("Are you sure you want to cancel this appointment?")) {
            query = "UPDATE APPOINTMENT SET status = 'Cancelled' WHERE appointmentId = " + to_string(appointmentId);

            if (mysql_query(conn, query.c_str()) == 0) {
                query = "UPDATE SLOT_TIME SET currentBookings = currentBookings - 1, isAvailable = TRUE WHERE slotTimeId = " + to_string(slotTimeId);
                mysql_query(conn, query.c_str());

                // Auto-release bay: Set bay status to Available
                query = "UPDATE SERVICE_BAY SET bayStatus = 'Available' WHERE serviceBayId = " + to_string(serviceBayId);
                mysql_query(conn, query.c_str());

                showSuccess("Appointment Cancelled Successfully.");
                showInfo("Bay released and available.");
            }
            else {
                showError("Error: " + string(mysql_error(conn)));
            }
        }

    }
    catch (OperationCancelledException&) {
    }
    pause();
}

// ============================================
// SCHEDULE APPOINTMENTS MENU
// ============================================
void scheduleAppointments() {
    int choice;
    do {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[36m1.\033[0m View Appointment Dashboard" << endl;
        cout << "\033[36m2.\033[0m Check Available Slots" << endl;
        cout << "\033[36m3.\033[0m Create New Booking" << endl;
        cout << "\033[36m4.\033[0m Update Appointment Status" << endl;
        cout << "\033[36m5.\033[0m Cancel Appointment" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 5);
            switch (choice) {
            case 1: setBreadcrumb("Home > Appointments > Dashboard"); viewAppointments(); break;
            case 2: setBreadcrumb("Home > Appointments > Available Slots"); viewAvailableSlots(); break;
            case 3: setBreadcrumb("Home > Appointments > Create Booking"); createAppointment(); break;
            case 4: setBreadcrumb("Home > Appointments > Update Status"); updateAppointmentStatus(); break;
            case 5: setBreadcrumb("Home > Appointments > Cancel"); cancelAppointment(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; break; }
    } while (choice != 0);
}
