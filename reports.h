#ifndef REPORTS_H
#define REPORTS_H

#include "globals.h"

// ============================================
// REPORT HELPER FUNCTIONS
// ============================================
void printReportHeader(const string& reportTitle, const string& startDate, const string& endDate);
void printReportFooter(int totalRecords);
void printSectionDivider(const string& sectionTitle);

// ============================================
// ACCESS CONTROL
// ============================================
bool checkReportAccess();

// ============================================
// REPORT FUNCTIONS (MANAGER ONLY)
// ============================================
void viewAppointmentSchedule();    // Detailed appointment schedule with services
void viewFinancialReport();        // Sales & revenue with service breakdown
void viewServiceTrends();          // Service popularity and trends analysis
void viewMechanicPerformance();    // Staff performance with job details
void viewCustomerAnalytics();      // VIP, frequent, dormant, growth analysis
void generateReports();            // Main reports menu

#endif // REPORTS_H
