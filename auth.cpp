#include "auth.h"
#include "utils.h"
#include "input_validation.h"

// ============================================
// AUTHENTICATION
// ============================================
bool login() {
    clearScreen();
    printSectionTitle("CAR SERVICE CENTER - LOGIN");

    int attempts = 0;
    while (attempts < 3) {
        try {
            string username = getValidString("Username", 1, 50, false);
            string password = getPasswordInput("Password");
            string hashedPassword = hashPassword(password);

            // Check DB
            string query = "SELECT staffId, fullName, role, isActive FROM STAFF WHERE username = '" + username +
                "' AND password = '" + hashedPassword + "'";

            if (mysql_query(conn, query.c_str())) {
                cout << "\033[31m[-] DB Error: " << mysql_error(conn) << endl;
                return false;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (mysql_num_rows(result) == 1) {
                MYSQL_ROW row = mysql_fetch_row(result);

                int status = atoi(row[3]);

                // Check Status specifically
                if (status == 0) {
                    cout << "\n\033[33m[!] ACCOUNT LOCKED: Your account has been deactivated.\033[0m" << endl;
                    cout << "\033[33m[!] Please contact the Administrator.\033[0m" << endl;
                    mysql_free_result(result);
                    pause();
                    clearScreen();
                    printSectionTitle("CAR SERVICE CENTER - LOGIN");

                    continue;
                }

                // Login Success
                currentStaffId = atoi(row[0]);
                currentStaffName = row[1];
                currentUserRole = row[2];
                isLoggedIn = true;

                mysql_free_result(result);

                cout << "\033[32m\n[+] Login Successful! Welcome, " << currentStaffName << " (" << currentUserRole << ")\033[0m" << endl;
                pause();
                return true;
            }
            else {
                cout << "\n\033[31m[-] Invalid Username or Password. Please try again.\033[0m" << endl;
                attempts++;
                cout << "\033[31mAttempts remaining: " << (3 - attempts) << "\033[0m\n" << endl;
            }
            mysql_free_result(result);
        }
        catch (OperationCancelledException&) {
            return false;
        }
    }

    cout << "\033[31m[-] Too many failed attempts. System exiting.\033[0m" << endl;
    return false;
}
