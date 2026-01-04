#include "appointment.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// VIEW AVAILABLE SLOTS
// ============================================
void viewAvailableSlots() {
    clearScreen();
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
    printSectionTitle("CREATE APPOINTMENT");

    try {
        // STEP 1: SELECT VEHICLE
        printSubHeader("Step 1: Select Vehicle");
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

        cout << "\n\033[1;97m=== Matching Vehicles ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(15) << "License" << setw(20) << "Vehicle" << setw(20) << "Owner" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(15) << row[1]
                << setw(20) << (string(row[2]) + " " + row[3]) << setw(20) << row[4] << endl;
        }
        mysql_free_result(res);

        int vehicleId = getValidInt("\nEnter Vehicle ID", 1, 99999);

        // STEP 2: SELECT SERVICES
        cout << endl;
        printSubHeader("Step 2: Select Services");
        query = "SELECT serviceTypeId, serviceName, standardDuration, basePrice FROM SERVICE_TYPE ORDER BY serviceName";
        mysql_query(conn, query.c_str());
        res = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Service" << setw(10) << "Mins" << setw(10) << "Price" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────────────────────────────" << "\033[0m" << endl;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(10) << row[2] << setw(10) << row[3] << endl;
        }
        mysql_free_result(res);

        int numServices = getValidInt("\nHow many services?", 1, 10);
        int serviceIds[10];
        int totalDuration = 0;

        for (int i = 0; i < numServices; i++) {
            serviceIds[i] = getValidInt("Enter Service ID #" + to_string(i + 1), 1, 999);

            query = "SELECT standardDuration FROM SERVICE_TYPE WHERE serviceTypeId = " + to_string(serviceIds[i]);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            if (row = mysql_fetch_row(res)) {
                totalDuration += atoi(row[0]);
            }
            mysql_free_result(res);
        }

        // STEP 3: DATE & TIME
        cout << endl;
        printSubHeader("Step 3: Schedule");

        string rawDate = getSmartDateInput("Enter Date (YYYYMMDD)", false);
        string date = "'" + rawDate + "'";

        query = "SELECT slotTimeId, slotTime, (maxCapacity - currentBookings) as avail FROM SLOT_TIME "
            "WHERE slotDate = " + date + " "
            "AND isAvailable = 1 "
            "AND (maxCapacity - currentBookings) > 0 "
            "AND (slotDate != CURDATE() OR slotTime > CURTIME()) "
            "ORDER BY slotTime";

        if (mysql_query(conn, query.c_str())) { showError("Invalid Date."); return; }
        res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) {
            showError("No slots available (or created) for " + rawDate + ".");
            mysql_free_result(res); pause(); return;
        }

        int validSlots[50];
        int slotCount = 0;

        cout << "\n\033[1;97m=== Available Slots for " << rawDate << " ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(6) << "No." << setw(12) << "Time" << setw(10) << "Left" << "\033[0m" << endl;
        cout << "\033[90m" << u8"────────────────────────────" << "\033[0m" << endl;
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

            for (int i = 0; i < numServices; i++) {
                string sQuery = "INSERT INTO APPOINTMENT_SERVICE (appointmentId, serviceTypeId, serviceStatus) VALUES ("
                    + to_string(appId) + ", " + to_string(serviceIds[i]) + ", 'Pending')";
                mysql_query(conn, sQuery.c_str());
            }

            mysql_query(conn, ("UPDATE SLOT_TIME SET currentBookings = currentBookings + 1 WHERE slotTimeId = " + to_string(slotTimeId)).c_str());

            showSuccess("Appointment Created Successfully! (ID: " + to_string(appId) + ")");
            showSuccess("Assigned to Bay ID: " + to_string(bayId));
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
    printSectionTitle("VIEW APPOINTMENTS");

    try {
        cout << "\033[1;97mView Filter Options:\033[0m" << endl;
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
            cout << "1. Scheduled\n2. In Progress\n3. Completed\n4. Cancelled\n";
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
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            while ((row = mysql_fetch_row(result))) {
                string mechName = (row[7] ? row[7] : "-");
                string status = row[6];
                string colorStatus = status;

                if (status == "In Progress") colorStatus = "\033[33mIn Progress\033[0m";
                else if (status == "Completed") colorStatus = "\033[32mCompleted\033[0m";
                else if (status == "Cancelled") colorStatus = "\033[31mCancelled\033[0m";

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

            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
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

        cout << "\n\033[1;97m=== Select Appointment ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "Date" << setw(10) << "Time"
            << setw(20) << "Customer" << setw(12) << "Plate" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(10) << row[2]
                << setw(20) << row[3] << setw(12) << row[4] << setw(15) << row[5] << endl;
        }
        mysql_free_result(res);

        int appointmentId = getValidInt("\nEnter Appointment ID from list above", 1, 99999);

        query = "SELECT status FROM APPOINTMENT WHERE appointmentId = " + to_string(appointmentId);
        if (mysql_query(conn, query.c_str())) return;
        res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) {
            showError("Invalid ID selected.");
            mysql_free_result(res);
            pause();
            return;
        }
        mysql_free_result(res);

        cout << "\n\033[1;97m=== Set New Status ===\033[0m" << endl;
        cout << "\033[36m1.\033[0m Scheduled" << endl;
        cout << "\033[36m2.\033[0m In Progress" << endl;
        cout << "\033[36m3.\033[0m Completed" << endl;
        cout << "\033[36m4.\033[0m Cancelled" << endl;

        int statusChoice = getValidInt("Enter choice", 1, 4);
        string status[] = { "", "Scheduled", "In Progress", "Completed", "Cancelled" };

        query = "UPDATE APPOINTMENT SET status = '" + status[statusChoice] + "' WHERE appointmentId = " + to_string(appointmentId);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            showSuccess("Status Updated Successfully to: " + status[statusChoice]);
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
        // PART A: MY ACTIVE JOBS
        string myJobQuery = "SELECT DISTINCT a.appointmentId, v.licensePlate, c.customerName, v.brand, v.model "
            "FROM APPOINTMENT_SERVICE aps "
            "JOIN APPOINTMENT a ON aps.appointmentId = a.appointmentId "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "WHERE aps.staffId = " + to_string(currentStaffId) + " "
            "AND aps.serviceStatus = 'In Progress'";

        if (mysql_query(conn, myJobQuery.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* myRes = mysql_store_result(conn);
        int myJobCount = mysql_num_rows(myRes);

        if (myJobCount > 0) {
            cout << "\033[36m" << u8"┌─── " << "\033[32m" << "MY ACTIVE JOBS (In Progress)" << "\033[36m" << u8" ───┐" << "\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "Plate" << setw(20) << "Customer" << setw(20) << "Vehicle" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(myRes))) {
                cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(20) << row[2]
                    << setw(20) << (string(row[3]) + " " + row[4]) << endl;
            }
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────" << "\033[0m" << endl;
        }
        else {
            showInfo("You have no active jobs right now.");
        }
        mysql_free_result(myRes);

        // PART B: JOB POOL
        cout << "\n\033[1;97m=== AVAILABLE JOB POOL (Today) ===\033[0m" << endl;

        string poolQuery = "SELECT a.appointmentId, v.licensePlate, c.customerName, st.slotTime, a.status "
            "FROM APPOINTMENT a "
            "JOIN VEHICLE v ON a.vehicleId = v.vehicleId "
            "JOIN CUSTOMER c ON v.customerId = c.customerId "
            "JOIN SLOT_TIME st ON a.slotTimeId = st.slotTimeId "
            "WHERE st.slotDate = CURDATE() "
            "AND a.status IN ('Scheduled', 'In Progress') "
            "ORDER BY st.slotTime ASC";

        if (mysql_query(conn, poolQuery.c_str())) { showError("Error: " + string(mysql_error(conn))); return; }
        MYSQL_RES* poolRes = mysql_store_result(conn);

        if (mysql_num_rows(poolRes) == 0) {
            showError("No active appointments in the shop today.");
        }
        else {
            cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "Plate" << setw(20) << "Customer"
                << setw(10) << "Time" << setw(15) << "Status" << "\033[0m" << endl;
            cout << "\033[90m" << u8"──────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(poolRes))) {
                cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(20) << row[2]
                    << setw(10) << row[3] << setw(15) << row[4] << endl;
            }
        }
        mysql_free_result(poolRes);

        // SELECT JOB
        showInfo("Enter Appointment ID to work on (or 0 to Exit)");
        int appointmentId = getValidInt("Enter ID", 0, 99999);

        if (appointmentId == 0) return;

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
            cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Service Task" << setw(12) << "Est."
                << setw(12) << "Actual" << setw(15) << "Status" << setw(20) << "Started At" << "\033[0m" << endl;
            cout << "\033[90m" << u8"────────────────────────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                string actual = (row[3] ? row[3] : "-");
                string startInfo = (row[5] ? row[5] : "-");

                cout << left << setw(5) << row[0] << setw(25) << row[1]
                    << setw(12) << (string(row[2]) + "m")
                    << setw(12) << (actual == "-" ? "-" : actual + "m")
                    << setw(15) << row[4]
                    << setw(20) << startInfo << endl;
            }
            mysql_free_result(res);

            showInfo("Enter Task ID to update status");
            int serviceId = getValidInt("Enter ID (or 0 to Finish/Back)", 0, 99999);

            if (serviceId == 0) break;

            cout << "\n1. Pending\n2. In Progress (Start Timer)\n3. Completed (Stop Timer)\n";
            int statusChoice = getValidInt("Select New Status", 1, 3);

            string updateQ = "";

            if (statusChoice == 2) {
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
                        "actualDuration = TIMESTAMPDIFF(MINUTE, startedAt, NOW()) "
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
                        showSuccess("Job Done. Returning to menu...");
                        pause();
                        break;
                    }
                }
                else {
                    if (statusChoice == 2) {
                        mysql_query(conn, ("UPDATE APPOINTMENT SET status = 'In Progress' WHERE appointmentId = " + to_string(appointmentId) + " AND status = 'Scheduled'").c_str());
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

        cout << "\n\033[1;97m=== Active Appointments ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(12) << "Date" << setw(10) << "Time"
            << setw(20) << "Customer" << setw(12) << "Plate" << setw(15) << "Status" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(12) << row[1] << setw(10) << row[2]
                << setw(20) << row[3] << setw(12) << row[4] << setw(15) << row[5] << endl;
        }
        mysql_free_result(res);

        int appointmentId = getValidInt("\nEnter Appointment ID to cancel", 1, 99999);

        query = "SELECT slotTimeId FROM APPOINTMENT WHERE appointmentId = " + to_string(appointmentId);
        if (mysql_query(conn, query.c_str())) return;
        res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) {
            showError("Invalid ID."); mysql_free_result(res); pause(); return;
        }

        row = mysql_fetch_row(res);
        int slotTimeId = atoi(row[0]);
        mysql_free_result(res);

        if (getConfirmation("Are you sure you want to cancel this appointment?")) {
            query = "UPDATE APPOINTMENT SET status = 'Cancelled' WHERE appointmentId = " + to_string(appointmentId);

            if (mysql_query(conn, query.c_str()) == 0) {
                query = "UPDATE SLOT_TIME SET currentBookings = currentBookings - 1, isAvailable = TRUE WHERE slotTimeId = " + to_string(slotTimeId);
                mysql_query(conn, query.c_str());

                showSuccess("Appointment Cancelled Successfully.");
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
        cout << "\n\033[1;97m2.0 SERVICE OPERATIONS\033[0m\n" << endl;
        cout << "\033[36m1.\033[0m View Appointment Dashboard" << endl;
        cout << "\033[36m2.\033[0m Check Available Slots" << endl;
        cout << "\033[36m3.\033[0m Create New Booking" << endl;
        cout << "\033[36m4.\033[0m Update Appointment Status" << endl;
        cout << "\033[36m5.\033[0m Cancel Appointment" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 5);
            switch (choice) {
            case 1: viewAppointments(); break;
            case 2: viewAvailableSlots(); break;
            case 3: createAppointment(); break;
            case 4: updateAppointmentStatus(); break;
            case 5: cancelAppointment(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; break; }
    } while (choice != 0);
}
