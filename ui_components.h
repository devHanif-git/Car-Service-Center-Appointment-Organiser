#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include "globals.h"
#include <windows.h>
#include <chrono>

// ============================================
// SESSION MANAGEMENT
// ============================================
extern std::chrono::steady_clock::time_point lastActivityTime;
const int SESSION_TIMEOUT_MINUTES = 30;

void updateLastActivity();
bool isSessionExpired();
void resetSession();

// ============================================
// STANDARDIZED MESSAGE FUNCTIONS
// ============================================
void showSuccess(const string& message);
void showError(const string& message);
void showWarning(const string& message);
void showInfo(const string& message);

// ============================================
// LOADING INDICATOR
// ============================================
void showLoadingStart(const string& message);
void showLoadingProgress(int current, int total);
void showLoadingComplete();

// ============================================
// NAVIGATION BREADCRUMBS
// ============================================
extern string currentBreadcrumb;
void setBreadcrumb(const string& path);
void displayBreadcrumb();

#endif // UI_COMPONENTS_H
