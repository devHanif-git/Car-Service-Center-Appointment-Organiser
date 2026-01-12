#ifndef INPUT_VALIDATION_H
#define INPUT_VALIDATION_H

#include "globals.h"
#include <functional>

// ============================================
// STRING HELPER
// ============================================
string trim(const string& str);

// ============================================
// INPUT VALIDATION FUNCTIONS
// ============================================
string getValidString(string prompt, int minLength = 1, int maxLength = 100, bool allowEmpty = false, std::function<void()> redrawCallback = nullptr);
int getValidInt(string prompt, int minVal, int maxVal, std::function<void()> redrawCallback = nullptr);
int getMenuChoice(string prompt, int minVal, int maxVal, std::function<void()> redisplayMenu);
string getValidPhone(string prompt);
string getValidEmail(string prompt);
string getValidYear(string prompt);
bool getConfirmation(string prompt);

// ============================================
// PASSWORD INPUT (Masked)
// ============================================
string getPasswordInput(string prompt);

#endif // INPUT_VALIDATION_H
