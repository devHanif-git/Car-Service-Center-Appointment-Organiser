#include "customer.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// ADD CUSTOMER
// ============================================
void addCustomer() {
    clearScreen();
    printSectionTitle("ADD NEW CUSTOMER");
    cout << endl;

    try {
        printSubHeader("Enter Customer Details");

        string name = getValidString("Enter Customer Name");
        string phone = getValidPhone("Enter Phone Number");

        // === DUPLICATION CHECK (PHONE) ===
        string checkPhone = "SELECT COUNT(*) FROM CUSTOMER WHERE phoneNumber = '" + phone + "'";
        mysql_query(conn, checkPhone.c_str());
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        if (atoi(row[0]) > 0) {
            showError("Phone number '" + phone + "' is already registered.");
            mysql_free_result(res);
            pause(); return;
        }
        mysql_free_result(res);

        string email = getValidEmail("Enter Email");

        // === DUPLICATION CHECK (EMAIL) ===
        string checkEmail = "SELECT COUNT(*) FROM CUSTOMER WHERE email = '" + email + "'";
        mysql_query(conn, checkEmail.c_str());
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        if (atoi(row[0]) > 0) {
            showError("Email '" + email + "' is already registered.");
            mysql_free_result(res);
            pause(); return;
        }
        mysql_free_result(res);

        string address = getValidString("Enter Address");

        // Insert into Database
        string query = "INSERT INTO CUSTOMER (customerName, phoneNumber, email, address, registrationDate) "
            "VALUES ('" + name + "', '" + phone + "', '" + email + "', '" + address + "', CURDATE())";

        if (mysql_query(conn, query.c_str())) {
            showError("Error adding customer: " + string(mysql_error(conn)));
        }
        else {
            int newId = mysql_insert_id(conn);

            showSuccess("Customer Added Successfully!");

            string confirmQuery = "SELECT customerId, customerName, phoneNumber, email, address FROM CUSTOMER "
                "WHERE customerId = " + to_string(newId);

            if (mysql_query(conn, confirmQuery.c_str()) == 0) {
                MYSQL_RES* result = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(result);
                
                cout << "\n\033[1;97m=== New Record in Database ===\033[0m" << endl;
                cout << "\033[36mID      :\033[0m " << row[0] << endl;
                cout << "\033[36mName    :\033[0m " << row[1] << endl;
                cout << "\033[36mPhone   :\033[0m " << row[2] << endl;
                cout << "\033[36mEmail   :\033[0m " << row[3] << endl;
                cout << "\033[36mAddress :\033[0m " << (row[4] ? row[4] : "-") << endl;

                mysql_free_result(result);
            }
        }

    }
    catch (OperationCancelledException&) {
    }
    setBreadcrumb("Home > Customers ");
    pause();
}

// ============================================
// SEARCH CUSTOMER
// ============================================
void searchCustomer() {
    clearScreen();
    printSectionTitle("CUSTOMER SEARCH");
    cout << endl;

    try {
        string term = getValidString("Enter Name, Phone, or Email to search");

        string query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
            "WHERE customerName LIKE '%" + term + "%' "
            "OR phoneNumber LIKE '%" + term + "%' "
            "OR email LIKE '%" + term + "%' "
            "ORDER BY customerName ASC";

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            showError("No records found matching '" + term + "'");
        }
        else {
            cout << "\n\033[1;97m=== Search Results (" << num_rows << " found) ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name"
                << setw(15) << "Phone" << setw(30) << "Email" << "\033[0m" << endl;
            cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result))) {
                cout << left << setw(5) << row[0] << setw(25) << row[1]
                    << setw(15) << row[2] << setw(30) << row[3] << endl;
            }
        }
        mysql_free_result(result);

    }
    catch (OperationCancelledException&) {}
    setBreadcrumb("Home > Customers");
    pause();
}

// ============================================
// VIEW CUSTOMERS
// ============================================
void viewCustomers() {
    clearScreen();
    printSectionTitle("VIEW ALL CUSTOMERS");
    cout << endl;

    MYSQL_RES* result;
    MYSQL_ROW row;

    int recordsPerPage = 10;
    int currentPage = 1;
    int totalRecords = 0;

    string countQuery = "SELECT COUNT(*) FROM CUSTOMER";
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
        showError("No customers found in the database.");
        pause();
        return;
    }

    int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;

    while (true) {
        clearScreen();
        printSectionTitle("VIEW ALL CUSTOMERS");
        cout << endl;

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
            "ORDER BY customerId LIMIT " + to_string(recordsPerPage) +
            " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
            pause();
            return;
        }

        result = mysql_store_result(conn);

        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name"
            << setw(15) << "Phone" << setw(30) << "Email" << "\033[0m" << endl;
        cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

        while ((row = mysql_fetch_row(result))) {
            cout << left << setw(5) << row[0] << setw(25) << row[1]
                << setw(15) << row[2] << setw(30) << row[3] << endl;
        }

        mysql_free_result(result);

        cout << "\n\033[90mPage " << currentPage << " of " << totalPages
            << " | Total: " << totalRecords << " customer(s)\033[0m" << endl;
        cout << "\n\033[36m[N]ext | [P]revious | [S]earch | [E]xit\033[0m" << endl;

        try {
            string input = getValidString("Enter choice", 1, 10, false);

            if (input.length() != 1) {
                showError("Invalid choice! Please enter N, P, S, or E.");
                pause();
                continue;
            }

            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'S') { searchCustomer(); return; }
            else if (choice == 'E') break;
            else {
                showError("Invalid choice! Please enter N, P, S, or E.");
                pause();
            }
        }
        catch (OperationCancelledException&) { break; }
    }
    setBreadcrumb("Home > Customers");
}

// ============================================
// UPDATE CUSTOMER
// ============================================
void updateCustomer() {
    clearScreen();
    printSectionTitle("UPDATE CUSTOMER");
    cout << endl;

    try {
        string term = getValidString("Enter Name, Phone, or Email to search");
        string query = "SELECT customerId, customerName, phoneNumber, email, address FROM CUSTOMER "
            "WHERE (customerName LIKE '%" + term + "%' OR phoneNumber LIKE '%" + term + "%' OR email LIKE '%" + term + "%')";

        if (mysql_query(conn, query.c_str())) { showError("Database error"); return; }
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { showError("No match found."); pause(); return; }

        cout << "\n\033[1;97m=== Matches ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "No." << setw(25) << "Name" << setw(15) << "Phone" << setw(30) << "Email" << "\033[0m" << endl;
        cout << "\033[90m" << u8"──────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        vector<int> customerIds;
        while ((row = mysql_fetch_row(res))) {
            customerIds.push_back(atoi(row[0]));
            cout << left << setw(5) << customerIds.size() << setw(25) << row[1] << setw(15) << row[2] << setw(30) << row[3] << endl;
        }
        mysql_free_result(res);

        int customerChoice = getValidInt("\nEnter Customer No.", 1, (int)customerIds.size());
        int id = customerIds[customerChoice - 1];

        // Fetch current customer data
        string fetchQuery = "SELECT customerId, customerName, phoneNumber, email, address FROM CUSTOMER WHERE customerId=" + to_string(id);
        if (mysql_query(conn, fetchQuery.c_str())) { showError("Database error"); return; }
        MYSQL_RES* currentRes = mysql_store_result(conn);
        MYSQL_ROW currentRow = mysql_fetch_row(currentRes);

        clearScreen();
        printSectionTitle("UPDATE CUSTOMER");
        cout << endl;

        // Display current customer data
        cout << "\n\033[1;97m=== Current Customer Data ===\033[0m" << endl;
        cout << "\033[36mID      :\033[0m " << currentRow[0] << endl;
        cout << "\033[36mName    :\033[0m " << currentRow[1] << endl;
        cout << "\033[36mPhone   :\033[0m " << currentRow[2] << endl;
        cout << "\033[36mEmail   :\033[0m " << currentRow[3] << endl;
        cout << "\033[36mAddress :\033[0m " << (currentRow[4] ? currentRow[4] : "-") << endl;
        mysql_free_result(currentRes);

        cout << "\033[90m" << u8"────────────────────────────────────────" << "\033[0m" << endl;
        cout << "\n\033[1;97m=== Select Field to Update ===\033[0m" << endl;
        cout << "\033[36m1.\033[0m Name" << endl;
        cout << "\033[36m2.\033[0m Phone" << endl;
        cout << "\033[36m3.\033[0m Email" << endl;
        cout << "\033[36m4.\033[0m Address" << endl;
        cout << "\033[36m5.\033[0m Update All" << endl;

        int choice = getValidInt("\nSelect Field", 1, 5);

        cout << endl;

        string updateQ = "UPDATE CUSTOMER SET ";
        if (choice == 1 || choice == 5) updateQ += "customerName='" + getValidString("New Name") + "',";
        if (choice == 2 || choice == 5) updateQ += "phoneNumber='" + getValidPhone("New Phone") + "',";
        if (choice == 3 || choice == 5) updateQ += "email='" + getValidEmail("New Email") + "',";
        if (choice == 4 || choice == 5) updateQ += "address='" + getValidString("New Address") + "',";

        updateQ.pop_back();
        updateQ += " WHERE customerId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            showError("Error: " + string(mysql_error(conn)));
        }
        else {
            clearScreen();
            printSectionTitle("UPDATE CUSTOMER");

            showSuccess("Customer Updated Successfully!");

            query = "SELECT customerId, customerName, phoneNumber, email, address FROM CUSTOMER WHERE customerId=" + to_string(id);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            cout << "\n\033[1;97m=== Updated Record ===\033[0m" << endl;
            cout << "\033[36mID      :\033[0m " << row[0] << endl;
            cout << "\033[36mName    :\033[0m " << row[1] << endl;
            cout << "\033[36mPhone   :\033[0m " << row[2] << endl;
            cout << "\033[36mEmail   :\033[0m " << row[3] << endl;
            cout << "\033[36mAddress :\033[0m " << (row[4] ? row[4] : "-") << endl;
            mysql_free_result(res);
        }

    }
    catch (OperationCancelledException&) {}
    setBreadcrumb("Home > Customers");
    pause();
}

// ============================================
// MANAGE CUSTOMER RECORDS MENU
// ============================================
void manageCustomerRecords() {
    int choice;

    auto displayMenu = [&]() {
        clearScreen();
        displayHeader();
        displayBreadcrumb();
        cout << "\n\033[36m1.\033[0m Search Customer Directory" << endl;
        cout << "\033[36m2.\033[0m Register New Customer" << endl;
        cout << "\033[36m3.\033[0m Update Customer Details" << endl;
        cout << "\033[36m4.\033[0m View Full Customer Registry" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;
    };

    do {
        displayMenu();

        try {
            choice = getMenuChoice("\nEnter choice", 0, 4, displayMenu);
            switch (choice) {
            case 1: setBreadcrumb("Home > Customers > Search"); searchCustomer(); break;
            case 2: setBreadcrumb("Home > Customers > Register"); addCustomer(); break;
            case 3: setBreadcrumb("Home > Customers > Update"); updateCustomer(); break;
            case 4: setBreadcrumb("Home > Customers > View All"); viewCustomers(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; break; }
    } while (choice != 0);
}
