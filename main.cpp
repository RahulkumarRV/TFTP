#include <iostream>
#include <thread>  // Include the thread header
#include <chrono>
using namespace std;

int evenSum = 0;
int oddSum = 0;

void evenSumFunction(int start, int end) {
    for (int i = start; i < end; i++) {
        if ((i & 1) == 0) {
            evenSum += i;
        }
    }
}

void oddSumFunction(int start, int end) {
    for (int i = start; i < end; i++) {
        if ((i & 1) == 1) {
            oddSum += i;
        }
    }
}

int main() {
    int start = 0, end = 190000000;
    auto starttime = std::chrono::high_resolution_clock::now();

    // Use std::thread to create threads
    std::thread t1(evenSumFunction, start, end);
    std::thread t2(oddSumFunction, start, end);

    t1.join();
    t2.join();
    // evenSumFunction(start, end);
    // oddSumFunction(start, end);

    auto endtime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = endtime - starttime;
    std::cout << "Time taken: " << duration.count() << " seconds\n";

    return 0;
}
