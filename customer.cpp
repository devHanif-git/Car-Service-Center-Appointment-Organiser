#include "customer.h"
#include "utils.h"
#include "input_validation.h"

// ============================================
// ADD CUSTOMER
// ============================================
void addCustomer() {
    clearScreen();
    printSectionTitle("ADD NEW CUSTOMER");

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
            cout << "\n\033[31m[-] Error: Phone number '" << phone << "' is already registered.\033[0m" << endl;
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
            cout << "\n\033[31m[-] Error: Email '" << email << "' is already registered.\033[0m" << endl;
            mysql_free_result(res);
            pause(); return;
        }
        mysql_free_result(res);

        string address = getValidString("Enter Address");

        // Insert into Database
        string query = "INSERT INTO CUSTOMER (customerName, phoneNumber, email, address, registrationDate) "
            "VALUES ('" + name + "', '" + phone + "', '" + email + "', '" + address + "', CURDATE())";

        if (mysql_query(conn, query.c_str())) {
            cout << "\n\033[31m[-] Error adding customer: " << mysql_error(conn) << endl;
        }
        else {
            int newId = mysql_insert_id(conn);

            cout << "\n\033[32m[+] Customer Added Successfully!\033[0m" << endl;

            string confirmQuery = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
                "WHERE customerId = " + to_string(newId);

            if (mysql_query(conn, confirmQuery.c_str()) == 0) {
                MYSQL_RES* result = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(result);

                cout << "\n\033[1;97m=== New Record in Database ===\033[0m" << endl;
                cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name"
                    << setw(15) << "Phone" << setw(30) << "Email" << "\033[0m" << endl;
                cout << "\033[90m" << u8"───────────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;

                cout << left << setw(5) << row[0] << setw(25) << row[1]
                    << setw(15) << row[2] << setw(30) << row[3] << endl;

                mysql_free_result(result);
            }
        }

    }
    catch (OperationCancelledException& e) {
    }

    pause();
}

// ============================================
// SEARCH CUSTOMER
// ============================================
void searchCustomer() {
    clearScreen();
    printSectionTitle("CUSTOMER SEARCH");

    try {
        string term = getValidString("Enter Name, Phone, or Email to search");

        string query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
            "WHERE customerName LIKE '%" + term + "%' "
            "OR phoneNumber LIKE '%" + term + "%' "
            "OR email LIKE '%" + term + "%' "
            "ORDER BY customerName ASC";

        if (mysql_query(conn, query.c_str())) {
            cout << "\033[31m[-] Error: " << mysql_error(conn) << endl;
            pause();
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            cout << "\n\033[31m[-] No records found matching '" << term << "'\033[0m" << endl;
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
    pause();
}

// ============================================
// VIEW CUSTOMERS
// ============================================
void viewCustomers() {
    clearScreen();
    printSectionTitle("VIEW ALL CUSTOMERS");

    MYSQL_RES* result;
    MYSQL_ROW row;

    int recordsPerPage = 10;
    int currentPage = 1;
    int totalRecords = 0;

    string countQuery = "SELECT COUNT(*) FROM CUSTOMER";
    if (mysql_query(conn, countQuery.c_str())) {
        cout << "\033[31m[-] Error: " << mysql_error(conn) << endl;
        pause();
        return;
    }

    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);
    totalRecords = atoi(row[0]);
    mysql_free_result(result);

    if (totalRecords == 0) {
        cout << "\n\033[31m[-] No customers found in the database.\033[0m" << endl;
        pause();
        return;
    }

    int totalPages = (totalRecords + recordsPerPage - 1) / recordsPerPage;

    while (true) {
        clearScreen();
        printSectionTitle("VIEW ALL CUSTOMERS");

        int offset = (currentPage - 1) * recordsPerPage;

        string query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER "
            "ORDER BY customerId LIMIT " + to_string(recordsPerPage) +
            " OFFSET " + to_string(offset);

        if (mysql_query(conn, query.c_str())) {
            cout << "\033[31m[-] Error: " << mysql_error(conn) << endl;
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
            string input = getValidString("Enter choice", 1, 1, false);
            char choice = toupper(input[0]);

            if (choice == 'N' && currentPage < totalPages) currentPage++;
            else if (choice == 'P' && currentPage > 1) currentPage--;
            else if (choice == 'S') { searchCustomer(); return; }
            else if (choice == 'E') break;
        }
        catch (OperationCancelledException&) { break; }
    }
}

// ============================================
// UPDATE CUSTOMER
// ============================================
void updateCustomer() {
    clearScreen();
    printSectionTitle("UPDATE CUSTOMER");

    try {
        string term = getValidString("Enter Name, Phone, or Email to search");
        string query = "SELECT customerId, customerName, phoneNumber, email, address FROM CUSTOMER "
            "WHERE (customerName LIKE '%" + term + "%' OR phoneNumber LIKE '%" + term + "%' OR email LIKE '%" + term + "%')";

        if (mysql_query(conn, query.c_str())) { cout << "\033[31m[-] Error\033[0m" << endl; return; }
        MYSQL_RES* res = mysql_store_result(conn);

        if (mysql_num_rows(res) == 0) { cout << "\033[31m[-] No match found.\033[0m" << endl; pause(); return; }

        cout << "\n\033[1;97m=== Matches ===\033[0m" << endl;
        cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name" << setw(15) << "Phone" << "\033[0m" << endl;
        cout << "\033[90m" << u8"──────────────────────────────────────────────" << "\033[0m" << endl;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(15) << row[2] << endl;
        }
        mysql_free_result(res);

        int id = getValidInt("\nEnter Customer ID", 1, 99999);

        cout << "\n1. Name\n2. Phone\n3. Email\n4. Address\n5. Update All\n";
        int choice = getValidInt("Select Field", 1, 5);

        string updateQ = "UPDATE CUSTOMER SET ";
        if (choice == 1 || choice == 5) updateQ += "customerName='" + getValidString("New Name") + "',";
        if (choice == 2 || choice == 5) updateQ += "phoneNumber='" + getValidPhone("New Phone") + "',";
        if (choice == 3 || choice == 5) updateQ += "email='" + getValidEmail("New Email") + "',";
        if (choice == 4 || choice == 5) updateQ += "address='" + getValidString("New Address") + "',";

        updateQ.pop_back();
        updateQ += " WHERE customerId=" + to_string(id);

        if (mysql_query(conn, updateQ.c_str())) {
            cout << "\033[31m[-] Error: " << mysql_error(conn) << endl;
        }
        else {
            cout << "\n\033[32m[+] Customer Updated Successfully!\033[0m" << endl;

            query = "SELECT customerId, customerName, phoneNumber, email FROM CUSTOMER WHERE customerId=" + to_string(id);
            mysql_query(conn, query.c_str());
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);

            cout << "\n\033[1;97m=== Updated Record ===\033[0m" << endl;
            cout << "\033[36m" << left << setw(5) << "ID" << setw(25) << "Name" << setw(15) << "Phone" << setw(25) << "Email" << "\033[0m" << endl;
            cout << "\033[90m" << u8"──────────────────────────────────────────────────────────────────────" << "\033[0m" << endl;
            cout << left << setw(5) << row[0] << setw(25) << row[1] << setw(15) << row[2] << setw(25) << row[3] << endl;
            mysql_free_result(res);
        }

    }
    catch (OperationCancelledException&) {}
    pause();
}

// ============================================
// MANAGE CUSTOMER RECORDS MENU
// ============================================
void manageCustomerRecords() {
    int choice;
    do {
        clearScreen();
        displayHeader();
        cout << "\n\033[1;97m4.0 CUSTOMER DATABASE\033[0m\n" << endl;
        cout << "\033[36m1.\033[0m Search Customer Directory" << endl;
        cout << "\033[36m2.\033[0m Register New Customer" << endl;
        cout << "\033[36m3.\033[0m Update Customer Details" << endl;
        cout << "\033[36m4.\033[0m View Full Customer Registry" << endl;
        cout << "\n\033[36m0.\033[0m Back to Main Menu" << endl;

        try {
            choice = getValidInt("\nEnter choice", 0, 4);
            switch (choice) {
            case 1: searchCustomer(); break;
            case 2: addCustomer(); break;
            case 3: updateCustomer(); break;
            case 4: viewCustomers(); break;
            case 0: break;
            }
        }
        catch (OperationCancelledException&) { choice = -1; pause(); }
    } while (choice != 0);
}
