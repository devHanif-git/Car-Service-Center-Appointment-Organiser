#include "database.h"
#include "ui_components.h"

// ============================================
// DATABASE CONNECTION
// ============================================
bool connectDatabase() {
    conn = mysql_init(NULL);

    if (conn == NULL) {
        showError("MySQL initialization failed!");
        return false;
    }

    if (mysql_real_connect(conn, "localhost", "root", "Asd123@sd",
        "car_service_mockup", 3306, NULL, 0) == NULL) {
        showError("Database connection failed!");
        cout << "Error: " << mysql_error(conn) << endl;
        mysql_close(conn);
        return false;
    }

    return true;
}
