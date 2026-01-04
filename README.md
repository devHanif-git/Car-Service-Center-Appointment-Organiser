# Car Service Center Organiser

A comprehensive Windows-based management system for automotive service centers, built with C++ and MySQL. This application streamlines workshop operations from customer management to appointment scheduling, service tracking, and business analytics.

---

## Table of Contents

- [Features](#features)
- [Technologies](#technologies)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [User Roles](#user-roles)
- [Database Schema](#database-schema)
- [Contributing](#contributing)
- [License](#license)

---

## Features

### Authentication & Security
- Role-based login system (Manager, Admin, Mechanic)
- SHA-256 password hashing via BCrypt
- 30-minute session timeout with inactivity tracking
- Account status management (active/deactivated)

### Customer Management
- Add, search, and view customer profiles
- Store contact information (name, phone, email, address)
- Duplicate prevention with phone/email validation
- Registration date tracking

### Vehicle Registry
- Associate vehicles with customers
- Track vehicle details (license plate, brand, model, year, color)
- View complete service history per vehicle
- License plate uniqueness validation

### Appointment Scheduling
- View available time slots with bay availability
- Multi-step appointment creation workflow
- Track appointment status (Scheduled, In Progress, Completed, Cancelled)
- Auto-generate 30-day appointment slots
- Cancellation with reason tracking

### Service Catalog
- Manage service types with pricing
- Track service duration and descriptions
- Search and update service offerings

### Service Bay Management
- Manage workshop bays/stations
- Real-time bay status tracking (Available, Occupied, Maintenance)
- View bay schedules and availability

### Staff Administration (Manager Only)
- Add new staff with role assignment
- Manage credentials and permissions
- View staff statistics and activity
- Deactivate or update staff information

### Analytics & Reports (Manager Only)
| Report Type | Description |
|-------------|-------------|
| Appointment Schedule | Detailed breakdown with services and status |
| Financial Report | Sales and revenue analysis by service type |
| Service Trends | Most popular services and frequency analysis |
| Mechanic Performance | Staff productivity and job completion rates |
| Customer Analytics | VIP customers, growth analysis, dormant accounts |

---

## Technologies

| Category | Technology |
|----------|------------|
| Language | C++ (C++11 or later) |
| Database | MySQL 8.0 |
| Build System | Visual Studio 2022 (v143 toolset) |
| Platform | Windows (x86 & x64) |
| Security | BCrypt (SHA-256 hashing) |
| DB Connector | MySQL C API |

### Libraries Used
- `mysql.h` - MySQL C API for database operations
- `bcrypt.h` - Password hashing
- `windows.h` - Windows API (console, datetime, UTF-8)
- Standard C++ libraries (iostream, string, vector, ctime, sstream, iomanip)

---

## Project Structure

```
Workshop1/
│
├── main.cpp                    # Application entry point
├── auth.cpp/.h                 # Login & authentication
├── database.cpp/.h             # MySQL connection
├── globals.cpp/.h              # Global session variables
├── menu.cpp/.h                 # Role-based menus
│
├── Core Entities
│   ├── customer.cpp/.h         # Customer management
│   ├── vehicle.cpp/.h          # Vehicle registry
│   ├── staff.cpp/.h            # Staff administration
│   ├── appointment.cpp/.h      # Appointment scheduling
│   ├── service_type.cpp/.h     # Service catalog
│   └── service_bay.cpp/.h      # Bay management
│
├── Business Features
│   ├── reports.cpp/.h          # Analytics & reporting
│   └── profile.cpp/.h          # User profile viewing
│
├── Utilities
│   ├── utils.cpp/.h            # UI, date, crypto helpers
│   ├── input_validation.cpp/.h # Input validation
│   └── ui_components.cpp/.h    # Standardized messaging
│
└── Build Files
    ├── Workshop1.vcxproj       # Visual Studio project
    ├── Workshop1.vcxproj.filters
    └── Workshop1.vcxproj.user
```

**Total**: ~4,800 lines of C++ code across 32 files

---

## Prerequisites

Before running this application, ensure you have:

1. **Visual Studio 2022** with C++ development tools
2. **MySQL Server 8.0** installed and running
3. **MySQL Connector/C** library configured
4. **BCrypt library** for password hashing

---

## Installation

### 1. Clone the Repository
```bash
git clone https://github.com/your-username/car-service-center.git
cd car-service-center/Workshop1
```

### 2. Set Up MySQL Database
```sql
CREATE DATABASE car_service_mockup;
USE car_service_mockup;

-- Run the provided SQL schema to create tables
-- (users, customers, vehicles, appointments, services, bays, etc.)
```

### 3. Configure Database Connection
Update the connection settings in `database.cpp`:
```cpp
// Default configuration
Host:     localhost
Port:     3306
Database: car_service_mockup
User:     your_mysql_user
Password: your_mysql_password
```

### 4. Build the Project
1. Open `Workshop1.vcxproj` in Visual Studio 2022
2. Select your configuration (Debug/Release) and platform (x86/x64)
3. Build the solution (`Ctrl + Shift + B`)

### 5. Run the Application
Execute the compiled binary from Visual Studio or run directly:
```bash
./x64/Debug/Workshop1.exe
```

---

## Usage

### Login
Upon launching, you'll be prompted to log in with your credentials:
```
╔══════════════════════════════════════════════════════════╗
║              CAR SERVICE CENTER ORGANISER                ║
╠══════════════════════════════════════════════════════════╣
║                        LOGIN                             ║
╚══════════════════════════════════════════════════════════╝

Username: admin
Password: ****
```

### Navigation
- Use numeric keys to select menu options
- Enter `@` at any prompt to cancel the current operation
- The system displays a breadcrumb trail showing your current location

### Special Input Features
- **Date Input**: Accepts multiple formats (YYYYMMDD, YYYY-MM-DD, YYYY/MM/DD)
- **Cancel Operation**: Type `@` to abort any action
- **Pagination**: Use `N` for Next, `P` for Previous when viewing lists

---

## User Roles

| Role | Access Level | Capabilities |
|------|--------------|--------------|
| **Manager** | Full Access | All features including staff management and reports |
| **Admin** | Operational | Customer, vehicle, appointment, service, and bay management |
| **Mechanic** | Limited | View job list, bay schedule, and personal profile |

### Role-Based Menu Structure

```
Manager Menu:
├── Customer Management
├── Vehicle Registry
├── Appointment Scheduling
├── Service Types
├── Service Bays
├── Staff Management
├── Reports & Analytics
└── Profile

Admin Menu:
├── Customer Management
├── Vehicle Registry
├── Appointment Scheduling
├── Service Types
├── Service Bays
└── Profile

Mechanic Menu:
├── Active Job List
├── Bay Schedule
└── Profile
```

---

## Database Schema

The system uses a relational database with the following core entities:

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  customers  │────<│  vehicles   │────<│appointments │
└─────────────┘     └─────────────┘     └─────────────┘
                                              │
                    ┌─────────────┐           │
                    │   users     │───────────┤
                    │  (staff)    │           │
                    └─────────────┘           │
                                              │
┌─────────────┐     ┌─────────────┐           │
│service_types│────<│service_bays │───────────┘
└─────────────┘     └─────────────┘
```

### Key Tables
- `users` - Staff accounts and credentials
- `customers` - Customer profiles
- `vehicles` - Vehicle registry linked to customers
- `appointments` - Scheduled services
- `service_types` - Available services with pricing
- `service_bays` - Workshop stations/bays

---

## Architecture

### Design Patterns

1. **Modular Architecture** - Separation of concerns into feature modules
2. **Layered Design**
   - Presentation Layer (UI components, menus)
   - Business Logic Layer (CRUD operations, validation)
   - Data Access Layer (MySQL queries)
   - Utility Layer (helpers, validation)

3. **RBAC (Role-Based Access Control)** - Menu and feature access based on user role
4. **Custom Exception Handling** - `OperationCancelledException` for user cancellations

### Security Features
- SHA-256 password hashing (no plaintext storage)
- Input validation and sanitization
- Session timeout after 30 minutes of inactivity
- Account lockout protection

---

## Console UI Features

The application features a polished console interface with:

- **ANSI Color Coding** - Status indicators and visual hierarchy
- **Box Drawing Characters** - Professional table and frame rendering
- **UTF-8 Support** - Proper character encoding
- **Breadcrumb Navigation** - Always know your current location
- **Standardized Messaging** - Consistent success, error, warning, and info displays

```
┌──────────────────────────────────────────────────────────┐
│ Status Legend:                                           │
│   [Scheduled]   - Awaiting service                       │
│   [In Progress] - Currently being serviced               │
│   [Completed]   - Service finished                       │
│   [Cancelled]   - Appointment cancelled                  │
└──────────────────────────────────────────────────────────┘
```

---

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Code Style Guidelines
- Use consistent indentation (4 spaces)
- Follow existing naming conventions
- Add comments for complex logic
- Test all changes before submitting

---

## License

This project is developed for educational purposes at UTeM (Universiti Teknikal Malaysia Melaka).

---

## Acknowledgments

- UTeM for the educational framework
- MySQL development team
- Visual Studio development team
- BCrypt library contributors

---

<p align="center">
  <strong>Car Service Center Organiser</strong><br>
  Built with C++ | Powered by MySQL
</p>
