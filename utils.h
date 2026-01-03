#ifndef UTILS_H
#define UTILS_H

#include "globals.h"
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

// ============================================
// CONSOLE INITIALIZATION
// ============================================
void initConsole();

// ============================================
// UI DISPLAY FUNCTIONS
// ============================================
void printSectionTitle(const string& title);
void printTableDivider(int length);
void printSubHeader(const string& title);
void displayHeader();

// ============================================
// STRING HELPERS
// ============================================
string repeatString(const string& str, int count);

// ============================================
// SCREEN UTILITIES
// ============================================
void clearScreen();
void pause();

// ============================================
// DATE HELPER FUNCTIONS
// ============================================
string getCurrentDateStr();
string formatSmartDate(string input);
string getSmartDateInput(string prompt, bool allowPastDates = true);
string addDaysToDate(string dateStr, int daysToAdd);

// ============================================
// AUTO SLOT GENERATION
// ============================================
void autoGenerateFutureSlots(int daysToMaintain);

// ============================================
// HELPER DISPLAY FUNCTION
// ============================================
void displayCustomerVehicles(int customerId);

// ============================================
// PASSWORD HASHING (SHA-256)
// ============================================
string hashPassword(const string& password);

#endif // UTILS_H
