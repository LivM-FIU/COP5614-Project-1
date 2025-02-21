#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "synch.h"

// Define Person Struct
struct Person {
    int id;
    int atFloor;
    int toFloor;
};

// Elevator Class
class Elevator {
public:
    int currentFloor;    // Track current floor
    int numFloors;       // Total floors in the building
    int occupancy;       // Current number of people inside the elevator
    int maxOccupancy;    // Maximum capacity (5)
    
    Lock *elevatorLock;  // Lock to prevent race conditions
    Condition *elevatorCondition; // Fixed missing declaration
    Condition **entering; // Condition variable array for people waiting to enter
    Condition **leaving;  // Condition variable array for people waiting to exit
    int *personsWaiting;  // Array to count how many people are waiting at each floor

    Elevator(int numFloors);   //  Renamed from `numFloors`
    ~Elevator();
    void start();           // Elevator movement logic
    void hailElevator(Person *p); // Handles passenger entry and exit
    bool noPendingRequests();  // Checks if requests exist
};

// Global Elevator Instance
extern Elevator *e;

// Person ID Generator
extern int nextPersonID;
extern Lock *nextPersonIDLock;

// Function Declarations
void ElevatorThread(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);

#endif // ELEVATOR_H
