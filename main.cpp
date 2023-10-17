#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

timed_mutex m;

int count = 0;

// try lock for a given time return a bool value if true then you can access to critical section other wise you continue to other part but not block
void print(int n, int number) {
    for (int i = 0; i < n; i++) {
        
        if(m.try_lock_for(chrono::seconds(2))){
            count++;
            cout << count << " " << number <<endl;
            this_thread::sleep_for(chrono::seconds(1));
            m.unlock();
        }
    }
}
0 > 1, 2
1 > 
2 > 2, 1
3 > 


int main() {
    thread t(print, 2, 1);
    thread t2(print, 2, 2);

    // Join the threads to wait for them to finish
    t.join();
    t2.join();

    cout << "count: " << count << endl;

    return 0;
}
