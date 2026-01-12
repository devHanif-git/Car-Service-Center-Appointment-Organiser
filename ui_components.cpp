#include "ui_components.h"

// ============================================
// SESSION MANAGEMENT
// ============================================
std::chrono::steady_clock::time_point lastActivityTime = std::chrono::steady_clock::now();
string currentBreadcrumb = "";

void updateLastActivity() {
    lastActivityTime = std::chrono::steady_clock::now();
}

bool isSessionExpired() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastActivityTime).count();
    return elapsed >= SESSION_TIMEOUT_MINUTES;
}

void resetSession() {
    lastActivityTime = std::chrono::steady_clock::now();
    currentBreadcrumb = "";
}

// ============================================
// STANDARDIZED MESSAGE FUNCTIONS
// ============================================
void showSuccess(const string& message) {
    cout << "\n\033[32m[SUCCESS] " << message << "\033[0m" << endl;
}

void showError(const string& message) {
    cout << "\n\033[31m[ERROR] " << message << "\033[0m" << endl;
}

void showWarning(const string& message) {
    cout << "\n\033[33m[WARNING] " << message << "\033[0m" << endl;
}

void showInfo(const string& message) {
    cout << "\n\033[36m[INFO] " << message << "\033[0m" << endl;
}

// ============================================
// LOADING INDICATOR
// ============================================
void showLoadingStart(const string& message) {
    cout << "\n\033[36m" << message << " \033[0m" << flush;
}

void showLoadingProgress(int current, int total) {
    int percent = (total > 0) ? (current * 100 / total) : 0;
    cout << "\n\r\033[36m[" << percent << "%] Processing... \033[0m" << flush;
}

void showLoadingComplete() {
    cout << "\n\r\033[32m[100%] Complete!              \033[0m" << endl;
}

// ============================================
// NAVIGATION BREADCRUMBS
// ============================================
void setBreadcrumb(const string& path) {
    currentBreadcrumb = path;
}

void displayBreadcrumb() {
    if (!currentBreadcrumb.empty()) {
        cout << "\033[90m" << u8">> " << currentBreadcrumb << "\033[0m" << endl;
    }
}
