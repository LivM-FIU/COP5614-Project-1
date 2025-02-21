#include "system.h"
#include "synch.h"
#include "elevator.h"
#include <stdint.h>  // Ensures intptr_t is defined

Elevator *e = nullptr;  // Global Elevator instance
int nextPersonID = 0; 
Lock *nextPersonIDLock = new Lock("PersonIDLock");  

// Elevator Constructor
Elevator::Elevator(int floors) { 
    this->numFloors = floors;
    this->currentFloor = 1;
    this->occupancy = 0;
    this->maxOccupancy = 5;

    // Initialize synchronization variables
    elevatorLock = new Lock("ElevatorLock");
    elevatorCondition = new Condition("ElevatorCondition");

    entering = new Condition*[numFloors]();
    leaving = new Condition*[numFloors]();
    personsWaiting = new int[numFloors]();

    for (int i = 0; i < numFloors; i++) {
        entering[i] = new Condition("Entering Floor");
        leaving[i] = new Condition("Leaving Floor");
        personsWaiting[i] = 0;
    }
}

// Elevator Start (Runs in a Thread)
void Elevator::start() {
    bool goingUp = true;

    while (true) {
        elevatorLock->Acquire();

        while (occupancy == 0 && noPendingRequests()) {
            printf("Elevator is idle, waiting for a request...\n");
            elevatorCondition->Wait(elevatorLock); 
        }

        for (int i = 0; i < numFloors; i++) {
            leaving[i]->Broadcast(elevatorLock);
        }

        for (int i = 0; i < numFloors; i++) {
            while (personsWaiting[i] > 0 && occupancy < maxOccupancy) {
                entering[i]->Signal(elevatorLock);
                occupancy++;
                personsWaiting[i]--;
            }
        }

        elevatorLock->Release();

        for (int j = 0; j < 50; j++) {
            currentThread->Yield();
        }

        if (goingUp) {
            if (currentFloor == numFloors) goingUp = false;
            else currentFloor++;
        } else {
            if (currentFloor == 1) goingUp = true;
            else currentFloor--;
        }

        printf("Elevator arrives at floor %d\n", currentFloor);
    }
}

// Handles a Passenger Requesting an Elevator Ride
void Elevator::hailElevator(Person *p) {
    elevatorLock->Acquire();

    printf("Person %d is waiting on floor %d.\n", p->id, p->atFloor);
    personsWaiting[p->atFloor - 1]++;
    elevatorCondition->Signal(elevatorLock); // Fixed missing variable

    entering[p->atFloor - 1]->Wait(elevatorLock);
    printf("Person %d got into the elevator.\n", p->id);

    occupancy++;

    leaving[p->toFloor - 1]->Wait(elevatorLock);
    occupancy--;

    printf("Person %d got out of the elevator.\n", p->id);
    elevatorLock->Release();
}

// Elevator Thread Function
void ElevatorThread(int numFloors) {
    if (e != nullptr) {
        printf("Elevator already initialized!\n");
        return;
    }
    printf("Elevator with %d floors was created!\n", numFloors);
    e = new Elevator(numFloors);
    e->start();
}

// Person Thread Function
void PersonThread(intptr_t personPtr) {
    Person *p = (Person *)personPtr;
    printf("👤 Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);
    e->hailElevator(p);
}

// Generates a Unique ID for Each Person
int getNextPersonID() {
    nextPersonIDLock->Acquire();
    int personID = nextPersonID++;
    nextPersonIDLock->Release();
    return personID;
}

// Creates a New Person and Requests the Elevator
void ArrivingGoingFromTo(int atFloor, int toFloor) {
    if (atFloor <= 0 || toFloor <= 0 || atFloor > e->numFloors || toFloor > e->numFloors || atFloor == toFloor) {
        printf("Invalid floor request: from %d to %d\n", atFloor, toFloor);
        return;
    }
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    char name[20];
    sprintf(name, "Person%d", p->id);
    Thread *t = new Thread(name);
    t->Fork(PersonThread, (intptr_t)p);
}

// Checks If There Are Any Pending Requests
bool Elevator::noPendingRequests() {
    for (int i = 0; i < numFloors; i++) {
        if (personsWaiting[i] > 0) return false;
    }
    return true;
}

Elevator::~Elevator() {
    delete elevatorLock;
    delete elevatorCondition;

    for (int i = 0; i < numFloors; i++) {
        delete entering[i];
        delete leaving[i];
    }

    delete[] entering;
    delete[] leaving;
    delete[] personsWaiting;
}
