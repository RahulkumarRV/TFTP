#include <iostream>
#include <thread>  // Include the thread header
#include <chrono>
using namespace std;

void print(int count){
    while(count --> 0){
        cout << "printing " << count << endl;
    }
    this_thread::sleep_for(std::chrono::seconds(3));
}


int main() {
    // program not execute after join until the thread not completed
    thread t(print, 5);
    cout << "join before print" << endl;
    t.join();
    cout << "join after print" << endl;
    return 0;
}
