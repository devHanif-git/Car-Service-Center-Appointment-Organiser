#include "auth.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// AUTHENTICATION
// ============================================
bool login() {
    clearScreen();
    printSectionTitle("CAR SERVICE CENTER - LOGIN");
    cout << endl;

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
                showError("DB Error: " + string(mysql_error(conn)));
                return false;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (mysql_num_rows(result) == 1) {
                MYSQL_ROW row = mysql_fetch_row(result);

                int status = atoi(row[3]);

                // Check Status specifically
                if (status == 0) {
                    showWarning("ACCOUNT LOCKED: Your account has been deactivated.");
                    showWarning("Please contact the Administrator.");
                    mysql_free_result(result);
                    pause();
                    clearScreen();
                    printSectionTitle("CAR SERVICE CENTER - LOGIN");
                    cout << endl;

                    continue;
                }

                // Login Success
                currentStaffId = atoi(row[0]);
                currentStaffName = row[1];
                currentUserRole = row[2];
                isLoggedIn = true;

                mysql_free_result(result);

                // Initialize session
                resetSession();

                showSuccess("Login Successful! Welcome, " + currentStaffName + " (" + currentUserRole + ")");
                pause();
                return true;
            }
            else {
                showError("Invalid Username or Password. Please try again.");
                attempts++;
                cout << "\033[31mAttempts remaining: " << (3 - attempts) << "\033[0m" << endl;
                mysql_free_result(result);

                if (attempts < 3) {
                    pause();
                    clearScreen();
                    printSectionTitle("CAR SERVICE CENTER - LOGIN");
                    cout << "\033[31mAttempts remaining: " << (3 - attempts) << "\033[0m\n" << endl;
                }
                continue;
            }
        }
        catch (OperationCancelledException&) {
            return false;
        }
    }

    showError("Too many failed attempts. System exiting.");
    return false;
}
