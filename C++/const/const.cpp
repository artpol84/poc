#include <iostream>

class internal {
public:
    int value;
    internal() {
        value = 0;
    }

    void inc() {
        value++;
    }
};


class outer {
public:
    internal *ptr;
    outer() {
        ptr = new internal;
    }
    
    void test() const {
        ptr->inc();
    }
};

int main()
{
    outer t;
    
    t.test();
    
    std::cout << "Value = " << t.ptr->value << std::endl;
}