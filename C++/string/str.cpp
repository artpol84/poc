#include <string>
#include <iostream>
#include <string.h>

using namespace std;

int main()
{
    string s, s1;
    char test[] = {0xda, 0xda, 0xda, 0xfa, 0x00, 0x00, 0xda, 0xda};
    
    s.resize(sizeof(test));
    memcpy((void*)s.data(), test, sizeof(test));
    
    s1.resize(2);
    memcpy((void*)s1.data(), &test[3], 2);
    
    s += "Trailing chars:2";

    cout << s.length() << endl;
    
    cout << "last ':'" << s.find_last_of(":") << endl;
    cout << "last '0xfa 0x00: " << s.find_last_of(s1) << endl;
    cout << "last '0xfa 0x00: " << s.find(s1) << endl;
    
    
}
