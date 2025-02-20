#include <chrono>
#include <iostream>
#include <thread>
#include <utility>
#include <mutex>
#include <list>

using namespace std;


class some_engine {
    volatile int val;
    thread::id progress_id;
    thread progress_thr;
    mutex m;
    list<int> prog_l, main_l;
    void cb_func();
public:

    void progress();

    void progress_func();

    some_engine();
    ~some_engine();

    void change_thread();
    
};

some_engine::some_engine()
{
    val = 0;
    new (&progress_thr) thread(&some_engine::progress_func, this);
    progress_id = progress_thr.get_id();
}

some_engine::~some_engine()
{
    progress_thr.join();
}

void some_engine::cb_func()
{
    list<int> &cur = main_l;
    int tmp;

    bool is_progress = (std::this_thread::get_id() == progress_id);
    cout << "Called from: " << ((is_progress) ? "PROGRESS" : "MAIN") << " thread" <<endl;
    if (is_progress) {
        cur = prog_l;
    }

    tmp = val;
    cur.push_back(tmp);
    val++;
}

void some_engine::progress()
{
    cb_func();
}

void some_engine::progress_func()
{
    int count;

    for(count = 0; count < 50; count++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m.lock();
        progress();
        m.unlock();
    }
}

void some_engine::change_thread()
{
    val++;
    m.lock();
    progress();
    m.unlock();
}

int main()
{
    some_engine e;

    for(int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        e.change_thread();
    }
    
}
