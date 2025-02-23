#include "system.h"
#include "synch.h"
#include "elevator.h"

ELEVATOR *e;
int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");

// Elevator Thread Function
void ElevatorThread(int numFloors) {
    printf("ELEVATOR with %d floors was created!\n", numFloors);
    e = new ELEVATOR(numFloors);
    e->start();
}

// Starts the Elevator Thread
void Elevator(int numFloors) {
    Thread *t = new Thread("ELEVATOR");
    t->Fork((VoidFunctionPtr)ElevatorThread, numFloors);
}

// Elevator Constructor
ELEVATOR::ELEVATOR(int floors) {
    numFloors = floors;
    currentFloor = 1;
    direction = 1;  // Start moving up
    occupancy = 0;
    maxOccupancy = 5;

    elevatorLock = new Lock("ElevatorLock");
    elevatorCondition = new Condition("ElevatorCondition");

    entering = new Condition*[numFloors];
    leaving = new Condition*[numFloors];
    personsWaiting = new int[numFloors];

    for (int i = 0; i < numFloors; i++) {
        entering[i] = new Condition("Entering Floor");
        leaving[i] = new Condition("Leaving Floor");
        personsWaiting[i] = 0;
    }
}

void ELEVATOR::start() {
    while (1) {
        elevatorLock->Acquire();

        bool hasPendingRequests = false;
        for (int i = 0; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasPendingRequests = true;
                break;
            }
        }

        // **Fix: Don't stop immediately - instead, wait for requests**
        if (occupancy == 0 && !hasPendingRequests) {
            printf("No passengers in the elevator. Waiting for requests...\n");
            elevatorCondition->Wait(elevatorLock);  // **Now it waits for passengers instead of stopping**
        }

        // **Re-check after waking up**
        hasPendingRequests = false;
        for (int i = 0; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasPendingRequests = true;
                break;
            }
        }

        // **Now stop ONLY if there are no passengers and no pending requests**
        if (occupancy == 0 && !hasPendingRequests) {
            printf("All passengers have exited. Stopping the elevator.\n");
            elevatorLock->Release();
            break;
        }

        printf("Elevator at floor %d: Checking for passengers to exit...\n", currentFloor);
        leaving[currentFloor - 1]->Broadcast(elevatorLock);

        for (int j = 0; j < 20; j++) {
            currentThread->Yield();
        }

        while (personsWaiting[currentFloor - 1] > 0 && occupancy < maxOccupancy) {
            printf("Elevator picking up a passenger at floor %d.\n", currentFloor);
            entering[currentFloor - 1]->Signal(elevatorLock);
            occupancy++;
            personsWaiting[currentFloor - 1]--;
            currentThread->Yield();
        }

        elevatorLock->Release();

        for (int j = 0; j < 50; j++) {
            currentThread->Yield();
        }

        elevatorLock->Acquire();
        bool hasUpRequests = false, hasDownRequests = false;
        for (int i = currentFloor; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasUpRequests = true;
                break;
            }
        }
        for (int i = 0; i < currentFloor - 1; i++) {
            if (personsWaiting[i] > 0) {
                hasDownRequests = true;
                break;
            }
        }

        if (!hasUpRequests && hasDownRequests) {
            direction = -1;
        } else if (hasUpRequests) {
            direction = 1;
        } else {
            direction = 0;
        }

        if (direction == 1 && currentFloor < numFloors) {
            currentFloor++;
        } else if (direction == -1 && currentFloor > 1) {
            currentFloor--;
        } else if (direction == 0) {
            printf("All requests handled. Stopping elevator.\n");
            elevatorLock->Release();
            break;
        }

        printf("Elevator arrives on floor %d\n", currentFloor);
        elevatorLock->Release();
        currentThread->Yield();
    }

    printf("Elevator has stopped. No more passengers.\n");
}


// Passenger Requests an Elevator Ride
void ELEVATOR::hailElevator(Person *p) {
    elevatorLock->Acquire();
    printf("Person %d is waiting on floor %d.\n", p->id, p->atFloor);
    personsWaiting[p->atFloor - 1]++;

    elevatorCondition->Broadcast(elevatorLock);
    entering[p->atFloor - 1]->Wait(elevatorLock);

    printf("Person %d got into the elevator.\n", p->id);
    occupancy++;

    leaving[p->toFloor - 1]->Wait(elevatorLock);
    occupancy--;

    elevatorLock->Release();
    printf("Person %d got out of the elevator.\n", p->id);
}

// Person Thread
void PersonThread(int person) {
    Person *p = (Person *)person;
    printf("Person %d wants to go from floor %d to floor %d\n", p->id, p->atFloor, p->toFloor);
    e->hailElevator(p);
}

// Generates Unique ID for Person
int getNextPersonID() {
    personIDLock->Acquire();
    int personID = nextPersonID++;
    personIDLock->Release();
    return personID;
}

// Creates a New Passenger Thread
void ArrivingGoingFromTo(int atFloor, int toFloor) {
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    Thread *t = new Thread("Person");
    t->Fork(PersonThread, (int)p);
}