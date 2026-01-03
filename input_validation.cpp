#include "input_validation.h"

// ============================================
// STRING HELPER
// ============================================
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// ============================================
// INPUT VALIDATION FUNCTIONS
// ============================================
string getValidString(string prompt, int minLength, int maxLength, bool allowEmpty) {
    string value;
    while (true) {
        cout << "\033[36m" << prompt << " (@ to cancel): \033[0m";
        getline(cin, value);

        // Check for Global Cancel
        if (trim(value) == "@") {
            cout << "\n\033[33m[!] Operation cancelled by user.\033[0m" << endl;
            throw OperationCancelledException();
        }

        // Handle Empty Input (for "Press Enter to skip/today")
        if (trim(value).empty()) {
            if (allowEmpty) return "";
            cout << "\033[31m[-] Input cannot be empty! Please try again.\033[0m" << endl;
            continue;
        }

        value = trim(value);

        if (value.length() < minLength) {
            cout << "\033[31m[-] Input too short! Minimum " << minLength << " characters.\033[0m" << endl;
        }
        else if (value.length() > maxLength) {
            cout << "\033[31m[-] Input too long! Maximum " << maxLength << " characters.\033[0m" << endl;
        }
        else {
            return value;
        }
    }
}

int getValidInt(string prompt, int minVal, int maxVal) {
    string input;
    int value;

    while (true) {
        cout << "\033[36m" << prompt << " (" << minVal << "-" << maxVal << ") (@ to cancel): \033[0m";
        getline(cin, input);

        if (trim(input) == "@") {
            cout << "\n\033[33m[!] Operation cancelled by user.\033[0m" << endl;
            throw OperationCancelledException();
        }

        try {
            value = stoi(input);
            if (value >= minVal && value <= maxVal) {
                return value;
            }
            else {
                cout << "\033[31m[-] Invalid number! Please enter between " << minVal << " and " << maxVal << ".\033[0m" << endl;
            }
        }
        catch (...) {
            cout << "\033[31m[-] Invalid input! Please enter a number.\033[0m" << endl;
        }
    }
}

string getValidPhone(string prompt) {
    string phone;
    while (true) {
        phone = getValidString(prompt, 10, 12, false);

        // Remove spaces and dashes for clean storage
        string cleanPhone;
        for (char c : phone) {
            if (isdigit(c)) cleanPhone += c;
        }

        if (cleanPhone.empty() || cleanPhone[0] != '0') {
            cout << "\033[31m[-] Phone number must start with 0!\033[0m" << endl;
        }
        else if (cleanPhone.length() < 10 || cleanPhone.length() > 11) {
            cout << "\033[31m[-] Phone number must be 10-11 digits!\033[0m" << endl;
        }
        else {
            return cleanPhone;
        }
    }
}

string getValidEmail(string prompt) {
    while (true) {
        string email = getValidString(prompt, 5, 50, false);

        if (email.find('@') == string::npos || email.find('.') == string::npos) {
            cout << "\033[31m[-] Invalid email format! Must contain '@' and domain.\033[0m" << endl;
        }
        else {
            return email;
        }
    }
}

string getValidYear(string prompt) {
    int year = getValidInt(prompt, 1900, 2030);
    return to_string(year);
}

bool getConfirmation(string prompt) {
    while (true) {
        string input = getValidString(prompt + " (Y/N)", 1, 3, false);
        char choice = toupper(input[0]);

        if (choice == 'Y') return true;
        if (choice == 'N') return false;

        cout << "\033[31m[-] Invalid input! Please enter Y or N.\033[0m" << endl;
    }
}

// ============================================
// PASSWORD INPUT (Masked)
// ============================================
string getPasswordInput(string prompt) {
    string password = "";
    char ch;
    cout << "\033[36m" << prompt << ": \033[0m";
    while (true) {
        ch = _getch();
        if (ch == 13) { cout << endl; break; } // Enter
        else if (ch == 8) { // Backspace
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        }
        else {
            password += ch;
            cout << "*";
        }
    }
    return password;
}
