#include "globals.h"

// ============================================
// GLOBAL VARIABLES DEFINITIONS
// ============================================
int currentStaffId = 0;
string currentStaffName = "";
string currentUserRole = "";
bool isLoggedIn = false;

// ============================================
// GLOBAL MySQL CONNECTION
// ============================================
MYSQL* conn;
