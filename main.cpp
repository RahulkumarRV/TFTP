#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex m;

int count = 0;

// try lock return a bool value if true then you can access to critical section other wise you continue to other part but not block
void print(int n, int number) {
    for (int i = 0; i < n; i++) {
        
        if(m.try_lock()){
            cout << count << " " << number <<endl;
            this_thread::sleep_for(chrono::seconds(1));
            m.unlock();
        }
    }
}

int main() {
    thread t(print, 2, 1);
    thread t2(print, 2, 2);

    // Join the threads to wait for them to finish
    t.join();
    t2.join();

    cout << "count: " << count << endl;

    return 0;
}
