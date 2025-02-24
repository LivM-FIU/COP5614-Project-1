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

        printf("🚀 DEBUG: Elevator at floor %d. Checking for passengers...\n", currentFloor);

        // ✅ Ensure elevator waits if there are no passengers inside and no new requests.
        bool hasPendingRequests = false;
        for (int i = 0; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasPendingRequests = true;
            }
        }

        if (occupancy == 0 && !hasPendingRequests) {
            printf("🛑 DEBUG: No passengers inside & no requests. Waiting for requests...\n");
            elevatorCondition->Wait(elevatorLock);

            // ✅ Check for new requests again after waking up
            hasPendingRequests = false;
            for (int i = 0; i < numFloors; i++) {
                if (personsWaiting[i] > 0) {
                    hasPendingRequests = true;
                    currentThread->Yield();

                }
            }

            if (occupancy == 0 && !hasPendingRequests) {
                printf("🛑 DEBUG: No more passengers. Stopping the ELEVATOR.\n");
                elevatorLock->Release();
            }
        }

        printf("🔄 DEBUG: Elevator at floor %d: Checking for passengers to exit...\n", currentFloor);
        leaving[currentFloor - 1]->Broadcast(elevatorLock);

        for (int j = 0; j < 20; j++) {
            currentThread->Yield();
        }

        printf("🚦 DEBUG: Checking if there are new passengers at floor %d...\n", currentFloor);
        while (personsWaiting[currentFloor - 1] > 0 && occupancy < maxOccupancy) {
            printf("✅ DEBUG: Elevator picking up a passenger at floor %d.\n", currentFloor);
            
            entering[currentFloor - 1]->Signal(elevatorLock);
            occupancy++;
            personsWaiting[currentFloor - 1]--;
            currentThread->Yield();
        }

        elevatorLock->Release();

        for (int j = 0; j < 50; j++) {
            currentThread->Yield();
        }

        // ✅ Check for new requests before moving
        elevatorLock->Acquire();
        hasPendingRequests = false;
        for (int i = 0; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasPendingRequests = true;
                break;
            }
        }

        // ✅ Don't move if new requests arrived
        if (hasPendingRequests) {
            printf("🚨 DEBUG: New passenger requests detected. Elevator will not stop.\n");
        }

        if (occupancy > 0 || hasPendingRequests) {
            if (currentFloor < numFloors) {
                currentFloor++;
            } else {
                currentFloor = 1;
            }
            printf("📍 DEBUG: ELEVATOR arrives on floor %d\n", currentFloor);
        }
        elevatorLock->Release();
    }

    printf("🛑 DEBUG: ELEVATOR has stopped. No more passengers.\n");
}


// Passenger Requests an Elevator Ride
void ELEVATOR::hailElevator(Person *p) {
    elevatorLock->Acquire();
    printf("🆕 DEBUG: Person %d is waiting on floor %d.\n", p->id, p->atFloor);
    personsWaiting[p->atFloor - 1]++;

    // ✅ Ensure the elevator wakes up immediately when a new person arrives
    printf("🔔 DEBUG: Person %d is waking up the elevator!\n", p->id);
    elevatorCondition->Signal(elevatorLock);

    entering[p->atFloor - 1]->Wait(elevatorLock);

    printf("🚪 DEBUG: Person %d got into the ELEVATOR.\n", p->id);
    occupancy++;

    leaving[p->toFloor - 1]->Wait(elevatorLock);
    occupancy--;

    elevatorLock->Release();
    printf("🏁 DEBUG: Person %d got out of the ELEVATOR.\n", p->id);
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