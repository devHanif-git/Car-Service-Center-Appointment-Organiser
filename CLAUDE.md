# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is a Visual Studio 2022 C++ project targeting Windows (x86/x64) with MySQL integration.

**Build from Visual Studio:**
- Open `Workshop1.vcxproj` in Visual Studio 2022
- Select configuration (Debug/Release) and platform (x86/x64)
- Build: `Ctrl + Shift + B`

**Run executable:**
```
.\x64\Debug\Workshop1.exe    # Debug build
.\x64\Release\Workshop1.exe  # Release build
```

**Dependencies:**
- MySQL Server 8.0 running locally on port 3306
- Database: `car_service_mockup`
- MySQL Connector/C (libmysql.lib)
- BCrypt library (bcrypt.lib) for password hashing

## Architecture Overview

### Application Flow
`main.cpp` initializes console, connects to database via `connectDatabase()`, then loops: `login()` → `mainMenu()` → logout/cleanup until exit.

### Layer Structure
1. **Presentation Layer**: `menu.cpp`, `ui_components.cpp` - role-based menus, breadcrumb navigation, standardized messages
2. **Business Logic Layer**: Feature modules (`customer.cpp`, `vehicle.cpp`, `appointment.cpp`, etc.) - CRUD operations and business rules
3. **Data Access Layer**: Direct MySQL C API calls within each module using global `MYSQL* conn`
4. **Utilities Layer**: `utils.cpp` (console/date helpers, SHA-256 hashing), `input_validation.cpp` (validated input functions)

### Global Session State (`globals.h`)
```cpp
extern int currentStaffId;
extern string currentStaffName;
extern string currentUserRole;  // "Manager", "Admin", "Mechanic"
extern bool isLoggedIn;
extern MYSQL* conn;
```

### Key Patterns

**Operation Cancellation**: All input functions check for `@` input and throw `OperationCancelledException`. Menu handlers catch this to return to menu without error.

**Input Validation Functions** (`input_validation.h`):
- `getValidString()`, `getValidInt()`, `getValidPhone()`, `getValidEmail()`, `getValidYear()`
- `getConfirmation()` - yes/no prompts
- `getPasswordInput()` - masked password entry

**UI Messaging** (`ui_components.h`):
- `showSuccess()`, `showError()`, `showWarning()`, `showInfo()` - consistent status messages
- `setBreadcrumb()` / `displayBreadcrumb()` - navigation trail
- 30-minute session timeout via `isSessionExpired()`

**Date Handling** (`utils.cpp`):
- `formatSmartDate()` - parses YYYYMMDD, YYYY-MM-DD, YYYY/MM/DD
- `getSmartDateInput()` - validated date input with format flexibility
- `autoGenerateFutureSlots()` - generates appointment slots 30 days ahead

### Role-Based Access Control
- **Manager**: Full access including staff management and all reports
- **Admin**: Operational access (customers, vehicles, appointments, services, bays)
- **Mechanic**: Limited view (job list, bay schedule, profile)

Menu options are conditionally rendered based on `currentUserRole`.

### Module Responsibilities
| Module | Purpose |
|--------|---------|
| `appointment.cpp` | Scheduling, status updates, slot management, bay assignment |
| `customer.cpp` | Customer profiles, duplicate prevention |
| `vehicle.cpp` | Vehicle registry linked to customers, service history |
| `service_type.cpp` | Service catalog with pricing/duration |
| `service_bay.cpp` | Bay status (Available/Occupied/Maintenance), scheduling |
| `staff.cpp` | Staff CRUD, role assignment (Manager only) |
| `reports.cpp` | Analytics: financial, service trends, performance, customer |
| `profile.cpp` | Current user profile view |
| `auth.cpp` | Login with password verification |

## Database Configuration

Connection settings in `database.cpp:15`:
```cpp
mysql_real_connect(conn, "localhost", "root", "password",
    "car_service_mockup", 3306, NULL, 0)
```

Update credentials as needed for local environment.
