#include <iostream>
#include <string>

class test {
    std::string s;
public:
    test(std::string _s) {
        s = _s;
    }
};

typedef test test_t;

int main()
{
    test_t *x = new test_t("abc");
    return 0;
}