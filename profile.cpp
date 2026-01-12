#include "profile.h"
#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// VIEW MY PROFILE
// ============================================
void viewMyProfile() {
    auto displayMenu = [&]() {
        clearScreen();
        displayBreadcrumb();
        printSectionTitle("MY PROFILE - " + currentStaffName);
        cout << endl;

        // 1. Fetch Latest Data
        string query = "SELECT staffId, fullName, username, role FROM STAFF WHERE staffId=" + to_string(currentStaffId);
        if (mysql_query(conn, query.c_str())) return;

        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        cout << "\033[36mID       :\033[0m " << row[0] << endl;
        cout << "\033[36mName     :\033[0m " << row[1] << endl;
        cout << "\033[36mUsername :\033[0m " << row[2] << endl;
        cout << "\033[36mRole     :\033[0m " << row[3] << endl;
        mysql_free_result(res);

        cout << "\n\033[90m" << u8"────────────────────────────" << "\033[0m" << endl;
        cout << "\033[36m1.\033[0m Change Password" << endl;
        cout << "\033[36m0.\033[0m Back to Main Menu" << endl;
    };

    while (true) {
        displayMenu();

        try {
            int choice = getMenuChoice("\nEnter choice", 0, 1, displayMenu);
            if (choice == 0) break;

            if (choice == 1) {
                // Change Password Logic
                cout << "\n\033[1;97m=== Change Password ===\033[0m" << endl;

                // 1. Verify Old Password
                string oldPass = getPasswordInput("Enter Current Password");
                string hashedOldPass = hashPassword(oldPass);

                // Verify against DB
                string verifyQ = "SELECT count(*) FROM STAFF WHERE staffId=" + to_string(currentStaffId) + " AND password='" + hashedOldPass + "'";
                mysql_query(conn, verifyQ.c_str());
                MYSQL_RES* res = mysql_store_result(conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                int valid = atoi(row[0]);
                mysql_free_result(res);

                if (valid == 0) {
                    showError("Incorrect current password!");
                    pause();
                    continue;
                }

                // 2. Set New Password
                string newPass = getPasswordInput("Enter New Password");
                string confirmPass = getPasswordInput("Confirm New Password");

                if (newPass != confirmPass) {
                    showError("Passwords do not match!");
                    pause();
                    continue;
                }

                if (newPass.length() < 4) {
                    showError("Password too short (min 4 chars).");
                    pause();
                    continue;
                }

                // 3. Update DB
                string hashedNewPass = hashPassword(newPass);
                string updateQ = "UPDATE STAFF SET password='" + hashedNewPass + "' WHERE staffId=" + to_string(currentStaffId);
                if (mysql_query(conn, updateQ.c_str())) {
                    showError("Error: " + string(mysql_error(conn)));
                }
                else {
                    showSuccess("Password changed successfully!");
                }
                pause();
            }
        }
        catch (OperationCancelledException&) { break; }
    }
    setBreadcrumb("Home");
}
