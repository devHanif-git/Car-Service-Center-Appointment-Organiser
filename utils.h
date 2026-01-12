#ifndef UTILS_H
#define UTILS_H

#include "globals.h"
#include <windows.h>
#include <bcrypt.h>
#include <functional>
#pragma comment(lib, "bcrypt.lib")

// ============================================
// CONSOLE INITIALIZATION
// ============================================
void initConsole();

// ============================================
// UI DISPLAY FUNCTIONS
// ============================================
void printSectionTitle(const string& title);
void printSubHeader(const string& title);
void displayHeader();

// ============================================
// STRING HELPERS
// ============================================
string repeatString(const string& str, int count);
void printSectionDivider(const string& sectionTitle, int width = 80, const string& titleColor = "\033[1;97m");

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
string getSmartDateInput(string prompt, bool allowPastDates = true, std::function<void()> redrawCallback = nullptr);
string addDaysToDate(string dateStr, int daysToAdd);

// ============================================
// AUTO SLOT GENERATION
// ============================================
void autoGenerateFutureSlots(int daysToMaintain);

// ============================================
// PASSWORD HASHING (SHA-256)
// ============================================
string hashPassword(const string& password);

#endif // UTILS_H
