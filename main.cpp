#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex m;

int count = 0;

// mutext help to lock the critical section, for more details refer to internet
void print(int n, int number) {
    for (int i = 0; i < n; i++) {
        m.lock();
        count++;
        cout<< count << " " << number << endl;
        this_thread::sleep_for(chrono::seconds(1));
        m.unlock();
        cout << number << " coming out " << endl;
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
