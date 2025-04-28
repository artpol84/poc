#include <iostream>
#include <memory>
#include <string>

using namespace std;

class A {
protected:
    string name;
public:
    A(string _name) : name(_name) {
        cout << name << ": A()" << endl;
    }

    virtual ~A() {
        cout << name << ": ~A()" << endl;
    }
};

class B : public A {
public:
    B(string _name) : A(_name) {
        cout << name << ": B()" << endl;
    }

    ~B() override {
        cout << name << ": ~B()" << endl;
    }
    
    void operate() {
        cout << name << ": B::operate()" << endl;
    }
};


unique_ptr<A> alloc(string name)
{
    return make_unique<B>(name);
}

void dealloc(unique_ptr<A> p)
{
    cout << "Exit dealloc" << endl;
}

void dealloc_cast(unique_ptr<A> p)
{
    A* rawBase = p.release();
    std::unique_ptr<B> np(static_cast<B*>(rawBase));
    np->operate();
    cout << "Exit dealloc_case" << endl;
}

int main()
{
    unique_ptr<A> a = alloc("a");
    unique_ptr<A> b = alloc("b");
    unique_ptr<A> c = alloc("c");

    cout << endl << "Done constructors" << endl;
    
    cout << endl << "Call dealloc" << endl;
    dealloc(std::move(b));

    cout << endl << "Call dealloc_cast" << endl;
    dealloc_cast(std::move(c));
    
    cout << endl << "Exit main" << endl;

    return 0;
}