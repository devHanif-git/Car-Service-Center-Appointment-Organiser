#ifndef INPUT_VALIDATION_H
#define INPUT_VALIDATION_H

#include "globals.h"

// ============================================
// STRING HELPER
// ============================================
string trim(const string& str);

// ============================================
// INPUT VALIDATION FUNCTIONS
// ============================================
string getValidString(string prompt, int minLength = 1, int maxLength = 100, bool allowEmpty = false);
int getValidInt(string prompt, int minVal, int maxVal);
string getValidPhone(string prompt);
string getValidEmail(string prompt);
string getValidYear(string prompt);
bool getConfirmation(string prompt);

// ============================================
// PASSWORD INPUT (Masked)
// ============================================
string getPasswordInput(string prompt);

#endif // INPUT_VALIDATION_H
