class Elevator {
    private:
        int currentFloor;
        int numFloors;
        Semaphore *elevatorLock;
    
    public:
        Elevator(int numFloors) {
            this->numFloors = numFloors;
            this->currentFloor = 0;
            elevatorLock = new Semaphore("ElevatorLock", 1);
        }
    
        void ArrivingGoingFromTo(int atFloor, int toFloor) {
            elevatorLock->P(); // Lock the elevator
    
            printf("Thread at floor %d wants to go to floor %d\n", atFloor, toFloor);
    
            // Move elevator to pick up the thread
            while (currentFloor != atFloor) {
                currentFloor += (currentFloor < atFloor) ? 1 : -1;
                currentThread->Yield();
            }
    
            // Move to destination
            while (currentFloor != toFloor) {
                currentFloor += (currentFloor < toFloor) ? 1 : -1;
                currentThread->Yield();
            }
    
            elevatorLock->V(); // Release the elevator
        }
    };
    