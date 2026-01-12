#include "utils.h"
#include "input_validation.h"
#include "ui_components.h"

// ============================================
// CONSOLE INITIALIZATION
// ============================================
void initConsole() {
    SetConsoleOutputCP(CP_UTF8);
}

// ============================================
// UI DISPLAY FUNCTIONS
// ============================================
void printSectionTitle(const string& title) {
    cout << "\033[36m" << u8"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓" << "\033[0m" << endl;

    int totalWidth = 43;
    int textLen = title.length();
    int padding = totalWidth - textLen;
    int leftPad = padding / 2;
    int rightPad = padding - leftPad;

    cout << "\033[36m" << u8"┃" << "\033[0m";
    for (int i = 0; i < leftPad; i++) cout << " ";
    cout << "\033[1;97m" << title << "\033[0m";
    for (int i = 0; i < rightPad; i++) cout << " ";
    cout << "\033[36m" << u8"┃" << "\033[0m" << endl;

    cout << "\033[36m" << u8"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛" << "\033[0m" << endl;
}

void printSubHeader(const string& title) {
    cout << "\033[36m" << u8"┌─── " << "\033[1;97m" << title << "\033[0;36m" << u8" ───┐" << "\033[0m" << endl;
}

void displayHeader() {
    cout << "\033[36m" << u8"╔═════════════════════════════════════╗" << "\033[0m" << endl;
    cout << "\033[36m" << u8"║" << "\033[0m" << "   " << "\033[1;97m" << "CAR SERVICE CENTER ORGANISER" << "\033[0m" << "      " << "\033[36m" << u8"║" << "\033[0m" << endl;
    cout << "\033[36m" << u8"╚═════════════════════════════════════╝" << "\033[0m" << endl;
}

// ============================================
// STRING HELPERS
// ============================================
string repeatString(const string& str, int count) {
    string result;
    for (int i = 0; i < count; i++) result += str;
    return result;
}

void printSectionDivider(const string& sectionTitle, int width, const string& titleColor) {
    int titleLen = (int)sectionTitle.length();
    int innerWidth = width - 2;
    int availableForDashes = innerWidth - titleLen - 2;
    int leftDashes = availableForDashes / 2;
    int rightDashes = availableForDashes - leftDashes;

    cout << "\n\033[36m" << u8"┌"
        << repeatString(u8"─", leftDashes)
        << " " << titleColor << sectionTitle << "\033[0;36m "
        << repeatString(u8"─", rightDashes)
        << u8"┐" << "\033[0m" << endl;
}

// ============================================
// SCREEN UTILITIES
// ============================================
void clearScreen() {
    system("cls");
}

void pause() {
    cout << "\n\033[90mPress Enter to continue...\033[0m";
    cin.get();
}

// ============================================
// DATE HELPER FUNCTIONS
// ============================================
string getCurrentDateStr() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];

    localtime_s(&tstruct, &now);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
    return string(buf);
}

string formatSmartDate(string input) {
    string clean = "";

    // A. Handle "YYYYMMDD" (Continuous numbers)
    if (input.length() == 8 && input.find_first_not_of("0123456789") == string::npos) {
        return input.substr(0, 4) + "-" + input.substr(4, 2) + "-" + input.substr(6, 2);
    }

    // B. Handle Separators (Space, Slash, Dot) -> Convert to Dash
    for (char c : input) {
        if (c == ' ' || c == '/' || c == '.') clean += '-';
        else clean += c;
    }

    // C. Pad Single Digits with Zero (e.g., 2025-1-1 -> 2025-01-01)
    stringstream ss(clean);
    string segment;
    vector<string> parts;

    while (getline(ss, segment, '-')) {
        parts.push_back(segment);
    }

    if (parts.size() == 3) {
        string year = parts[0];
        string month = parts[1];
        string day = parts[2];

        if (month.length() == 1) month = "0" + month;
        if (day.length() == 1) day = "0" + day;

        return year + "-" + month + "-" + day;
    }

    return input;
}

string getSmartDateInput(string prompt, bool allowPastDates, function<void()> redrawCallback) {
    bool firstRun = true;
    while (true) {
        // Redraw screen if callback provided (skip on first run)
        if (redrawCallback && !firstRun) {
            pause();
            clearScreen();
            redrawCallback();
        }
        firstRun = false;

        cout << "\033[36m" << prompt << " [Enter for Today or @ to cancel]: \033[0m";
        string input;
        getline(cin, input);

        // Handle Global Cancel
        if (trim(input) == "@") {
            showWarning("Operation cancelled by user.");
            pause();
            throw OperationCancelledException();
        }

        // 1. Handle Empty -> Return Today
        if (trim(input).empty()) {
            string today = getCurrentDateStr();
            cout << "\033[90m    [Selected: " << today << "]\033[0m" << endl;
            return today;
        }

        // 2. Smart Format
        string formatted = formatSmartDate(trim(input));

        // 3. Basic Validation (Length must be 10: YYYY-MM-DD)
        if (formatted.length() != 10 || formatted[4] != '-' || formatted[7] != '-') {
            showError("Invalid format. Try 'YYYY-MM-DD' or 'YYYY 1 1'.");
            continue;
        }

        // 4. Past Date Validation (if not allowed)
        if (!allowPastDates) {
            string today = getCurrentDateStr();
            if (formatted < today) {
                showError("Date cannot be in the past. Please enter today or a future date.");
                continue;
            }
        }

        return formatted;
    }
}

string addDaysToDate(string dateStr, int daysToAdd) {
    int y, m, d;
    if (sscanf_s(dateStr.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return "";

    struct tm t = { 0 };
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d + daysToAdd;

    mktime(&t);

    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
    return string(buf);
}

// ============================================
// AUTO SLOT GENERATION
// ============================================
void autoGenerateFutureSlots(int daysToMaintain) {
    showInfo("[System] Checking schedule health...");

    // 1. Get Today's Date
    string today = getCurrentDateStr();

    // 2. Find the furthest existing date in the DB
    string query = "SELECT MAX(slotDate) FROM SLOT_TIME";
    if (mysql_query(conn, query.c_str())) return;

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    string lastDbDate = (row[0]) ? string(row[0]) : today;
    mysql_free_result(res);

    // 3. Calculate Target Date (Today + X days)
    string targetDate = addDaysToDate(today, daysToMaintain);

    // 4. Compare: Do we need to generate?
    if (lastDbDate >= targetDate) {
        showSuccess("[System] Schedule is up-to-date (Slots available until " + lastDbDate + ").");
        return;
    }

    // 5. Generation Loop
    showInfo("[System] Extending schedule from " + lastDbDate + " to " + targetDate + "...");

    string nextDate = addDaysToDate(lastDbDate, 1);
    int daysGenerated = 0;
    int capacity = 3;

    // Calculate total days to generate for progress bar
    int totalDays = 0;
    string tempDate = nextDate;
    while (tempDate <= targetDate) {
        totalDays++;
        tempDate = addDaysToDate(tempDate, 1);
    }

    // Last slot is 16:30 - services must complete by 17:15 (15-min buffer after 17:00 closing)
    string timeSlots[] = { "08:00:00", "08:30:00", "09:00:00", "09:30:00",
                           "10:00:00", "10:30:00", "11:00:00", "11:30:00",
                           "12:00:00", "12:30:00", "13:00:00", "13:30:00",
                           "14:00:00", "14:30:00", "15:00:00", "15:30:00",
                           "16:00:00", "16:30:00" };

    showLoadingStart("Generating time slots");
    while (nextDate <= targetDate) {
        for (const string& time : timeSlots) {
            string insertQ = "INSERT INTO SLOT_TIME (slotDate, slotTime, maxCapacity, currentBookings, isAvailable) "
                "VALUES ('" + nextDate + "', '" + time + "', " + to_string(capacity) + ", 0, 1)";
            mysql_query(conn, insertQ.c_str());
        }

        daysGenerated++;
        showLoadingProgress(daysGenerated, totalDays);
        nextDate = addDaysToDate(nextDate, 1);
    }
    showLoadingComplete();

    showSuccess("[System] Generated slots for " + to_string(daysGenerated) + " new days.");
    pause();
}

// ============================================
// PASSWORD HASHING (SHA-256)
// ============================================
string hashPassword(const string& password) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbData = 0, cbHashObject = 0, cbHash = 0;
    PBYTE pbHashObject = NULL;
    PBYTE pbHash = NULL;
    string result = "";

    // Open algorithm provider
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0) {
        return password; // fallback to plain if hash fails
    }

    // Get hash object size
    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    // Get hash size
    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
    pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHash);

    if (!pbHashObject || !pbHash) {
        if (pbHashObject) HeapFree(GetProcessHeap(), 0, pbHashObject);
        if (pbHash) HeapFree(GetProcessHeap(), 0, pbHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    // Create hash object
    if (BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0) != 0) {
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        HeapFree(GetProcessHeap(), 0, pbHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    // Hash the password
    if (BCryptHashData(hHash, (PBYTE)password.c_str(), (ULONG)password.length(), 0) != 0) {
        BCryptDestroyHash(hHash);
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        HeapFree(GetProcessHeap(), 0, pbHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    // Finish hash
    if (BCryptFinishHash(hHash, pbHash, cbHash, 0) != 0) {
        BCryptDestroyHash(hHash);
        HeapFree(GetProcessHeap(), 0, pbHashObject);
        HeapFree(GetProcessHeap(), 0, pbHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return password;
    }

    // Convert to hex string
    char hex[3];
    for (DWORD i = 0; i < cbHash; i++) {
        sprintf_s(hex, "%02x", pbHash[i]);
        result += hex;
    }

    // Cleanup
    BCryptDestroyHash(hHash);
    HeapFree(GetProcessHeap(), 0, pbHashObject);
    HeapFree(GetProcessHeap(), 0, pbHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return result;
}
