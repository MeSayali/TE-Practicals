
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

int buff = 0;

// Wait operation (P)
void wait(int& s) { //means  resource is unavailable.
    if (s <= 0) {
        throw runtime_error("Semaphore underflow: resource unavailable!");
    } else {
        s--;//esource available then we decrease it measn it s taken
    }
}

// Signal operation (V)
void signal(int& s) {//resources is release
    s++;
}

int main() {
    int emp = 1;
    int full = 0;
    int mutex = 1;

    try {
        for (int i = 1; i <= 5; ++i) {
            wait(emp);
            wait(mutex);//enter

            buff = i;
            cout << "Produced item: " << buff << endl;

            signal(mutex);//exit
            signal(full);

            if (i == 3) {
                full = 0; 
            }

            wait(full); // Will throw on i = 3
            wait(mutex);//enter

            cout << "Consumed item: " << buff << endl;
            buff = 0;

            signal(mutex);
            signal(emp);
        }
    } catch (const exception& e) {
        cerr << "[EXCEPTION] " << e.what() << endl;
    }

    return 0;
}
