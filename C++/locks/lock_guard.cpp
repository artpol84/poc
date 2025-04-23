#include <iostream>
#include <mutex>

using namespace std;

class myLock {
    public:
        myLock() {}
        void lock() {
            cout << "\tLOCKING" << endl;
        }
        void unlock() {
            cout << "\tunLOCKING" << endl;
        }
};




void f(myLock &l, int in)
{
    cout << "START:" << endl;
    std::lock_guard<myLock> lock_guard(l);
    
    cout << "\t0:" << endl;
    if (in == 0){
        return;
    }

    cout << "\t1:" << endl;
    if (in == 1){
        return;
    }

    cout << "\t2:" << endl;
    if (in == 2){
        return;
    }

    cout << "\t3+:" << endl;
    return;
}

int main()
{
    myLock l;
    f(l, 0);
    f(l, 1);
    f(l, 2);
    f(l, 3);
}