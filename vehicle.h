#ifndef VEHICLE_H
#define VEHICLE_H

#include "globals.h"

// ============================================
// HELPER DISPLAY FUNCTION
// ============================================
void displayCustomerVehicles(int customerId);

// ============================================
// VEHICLE MANAGEMENT FUNCTIONS
// ============================================
void addVehicle();
void searchVehicle();
void viewVehicles();
void updateVehicle();
void viewVehicleServiceHistory();
void trackVehicleRecords();

#endif // VEHICLE_H
