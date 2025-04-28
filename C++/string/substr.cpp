#include <string>
#include <iostream>

using namespace std;

int main()
{
    std::string str = "agent_name:10:20:1";
    
    size_t pos1 = str.find(":");
    size_t pos2 = str.find(":", pos1);
    size_t pos3 = str.find(":", pos1 + 1);
    size_t pos4 = str.find(":", pos3 + 1);
    size_t pos5 = str.find(":", pos4 + 1);
    
    cout << "pos1=" << pos1 << "pos2=" << pos2 << "pos3=" << pos3 << "pos4=" << pos4 << "pos5=" << pos5 << endl;
    
    cout << "last of = " << str.find_last_of(":") << endl;
    cout << "fullname=" << str << " trimmed=" << str.substr(0, str.find_last_of(":")) << endl;
    
}