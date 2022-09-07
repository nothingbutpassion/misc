#include <string>
#include <iostream>

using namespace std;

int main()
{
    string s0;
    string s1 = "x";
    string s2 = "xy";
	string s3 = "y";
	
	cout << s0 << " < " << s1 << " = " <<(s0 < s1) << endl;
	cout << s1 << " < " << s2 << " = " <<(s1 < s2) << endl;
	cout << s2 << " < " << s3 << " = " <<(s2 < s3) << endl;
	
	cout << s0 << " compare " << s1 << " = " << s0.compare(s1) << endl;
	cout << s1 << " compare " << "x" << " = " << s1.compare("x") << endl;
	cout << s3 << " compare " << s2 << " = " << s3.compare(s2) << endl;
	
    return 0;
}
