#include "database.h"

// ============================================
// DATABASE CONNECTION
// ============================================
bool connectDatabase() {
    conn = mysql_init(NULL);

    if (conn == NULL) {
        cout << "\n\033[31m[-] MySQL initialization failed!\033[0m" << endl;
        return false;
    }

    if (mysql_real_connect(conn, "localhost", "root", "Asd123@sd",
        "car_service_mockup", 3306, NULL, 0) == NULL) {
        cout << "\n\033[31m[-] Database connection failed!\033[0m" << endl;
        cout << "Error: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return false;
    }

    return true;
}
