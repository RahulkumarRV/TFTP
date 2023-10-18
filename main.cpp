#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

condition_variable cv;
mutex m;
int balance = 0;

void addMoney(int money){
    lock_guard<mutex> lg(m);
    balance += money;
    cout << "Amount Added Currect Balance: " << balance << endl;
    cv.notify_one();
}

void withdrawMoney(int money){
    unique_lock<mutex> ul(m);
    cv.wait(ul, [] { return (balance != 0) ? true : false; });
    if(balance >= money) {
        balance -= money;
        cout << "Amount Deducted Currect Balance: " << balance << endl;
    }else{
        cout << "Amount can't be deducted. Current balance is lesser than"<< money << endl;
    }

    cout << "current balance: " << balance << endl;

}

int main() {
    thread t(withdrawMoney, 600);
    thread t2(addMoney, 500);
    t.join();
    t2.join();

    return 0;
}
