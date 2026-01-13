#ifndef GLOBALS_H
#define GLOBALS_H

#define NOMINMAX

#include <iostream>
#include <mysql.h>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cctype>
#include <conio.h>
#include <ctime>
#include <sstream>
#include <vector>
#include <limits>

using namespace std;

// ============================================
// CUSTOM EXCEPTION
// ============================================
struct OperationCancelledException {};

// ============================================
// GLOBAL VARIABLES (Session State)
// ============================================
extern int currentStaffId;
extern string currentStaffName;
extern string currentUserRole;
extern bool isLoggedIn;

// ============================================
// GLOBAL MySQL CONNECTION
// ============================================
extern MYSQL* conn;

#endif // GLOBALS_H
