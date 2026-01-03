#include "profile.h"
#include "utils.h"
#include "input_validation.h"

// ============================================
// VIEW MY PROFILE
// ============================================
void viewMyProfile() {
    while (true) {
        clearScreen();
        printSectionTitle("MY PROFILE - " + currentStaffName);

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

        try {
            int choice = getValidInt("\nEnter choice", 0, 1);
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
                res = mysql_store_result(conn);
                row = mysql_fetch_row(res);
                int valid = atoi(row[0]);
                mysql_free_result(res);

                if (valid == 0) {
                    cout << "\n\033[31m[-] Incorrect current password!\033[0m" << endl;
                    pause();
                    continue;
                }

                // 2. Set New Password
                string newPass = getPasswordInput("Enter New Password");
                string confirmPass = getPasswordInput("Confirm New Password");

                if (newPass != confirmPass) {
                    cout << "\n\033[31m[-] Passwords do not match!\033[0m" << endl;
                    pause();
                    continue;
                }

                if (newPass.length() < 4) {
                    cout << "\n\033[31m[-] Password too short (min 4 chars).\033[0m" << endl;
                    pause();
                    continue;
                }

                // 3. Update DB
                string hashedNewPass = hashPassword(newPass);
                string updateQ = "UPDATE STAFF SET password='" + hashedNewPass + "' WHERE staffId=" + to_string(currentStaffId);
                if (mysql_query(conn, updateQ.c_str())) {
                    cout << "\033[31m[-] Error: " << mysql_error(conn) << endl;
                }
                else {
                    cout << "\n\033[32m[+] Password changed successfully!\033[0m" << endl;
                }
                pause();
            }
        }
        catch (OperationCancelledException&) { break; }
    }
}
