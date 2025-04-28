#include <iostream>
#include <vector>

using namespace std;

int main()
{
    vector<int> a,b;
    
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    b.push_back(4);
    b.push_back(5);
    
    move(b.begin(), b.end(), back_inserter(a));
    b.erase(b.begin(), b.end());
    
    cout << "a size = " << a.size() << endl;
    for(int i = 0; i < a.size(); i++){
	cout << "a[" << i << "] = " << a[i] << endl;
    }
}
