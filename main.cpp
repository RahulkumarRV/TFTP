#include <iostream>
#include <thread>  // Include the thread header
#include <chrono>
using namespace std;

void print(int count){
    while(count --> 0){
        cout << "printing " << count << endl;
    }
    this_thread::sleep_for(std::chrono::seconds(3));
    cout << "print function completed" << endl;
}


int main() {
    
    thread t(print, 5);
    t.detach(); // seperate the this thread from the main thread, this thread excutes independently
    cout<< "Main thread completed" << endl;
    this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
