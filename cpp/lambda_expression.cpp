#include <iostream>
#include <list>
#include <vector>
#include <functional>
#include <algorithm>


using namespace std;


class Scale
{
public:
    // The constructor.
    explicit Scale(int scale)
    : _scale(scale)
    {
    }
  
    // Prints the product of each element in a vector object 
    // and the scale value to the console.
    void ApplyScale(const vector<int>& v)
    {
		//// Capture this
		//for_each(v.begin(), v.end(), 
		//	[this](int n) { cout << n * _scale++ << endl; });

		////  Use the this pointer explicitly in a method
		//for_each(v.begin(), v.end(), 
		//	[this](int n) { cout << n * this->_scale++ << endl; });

		// Capture the this pointer implicitly by value
		// This pointer can't be modified but the _scale member can be modified
		for_each(v.begin(), v.end(), 
			[=](int n) { cout << n * _scale++ << endl; });

		cout << "scale = " << _scale << endl;

		// Capture the this pointer implicitly by reference
		// Both this pointer and the _scale member can be modified
		for_each(v.begin(), v.end(), 
			[&](int n) { cout << n * _scale++ << endl; });

		cout << "scale = " << _scale << endl;

    }
  
private:
    int _scale;
};


// Negates each element in the vector object.
template <typename T> 
void negate_all(vector<T>& v)
{
    for_each(v.begin(), v.end(), [] (T& n) { n = -n; } );
}
  
// Prints to the console each element in the vector object.
template <typename T> 
void print_all(const vector<T>& v)
{
    for_each(v.begin(), v.end(), [] (const T& n) { cout << n << endl; } );
}


int main(int argc, char* argv[])
{

	 // Assign the lambda expression that adds two numbers to an auto variable.
     auto f1 = [] (int x, int y) { return x + y; }; 
  
     // Assign the same lambda expression to a function object.
     function<int (int, int)> f2 = [] (int x, int y) { return x + y; };

	 // Invoke the function object and print its result.
     cout << "f1(21, 12) = " << f1(21, 12) << endl;
	 cout << "f2(31, 13) = " << f2(31, 13) << endl;

	 int i = 3;
     int j = 5;
  
     // The following lambda expression captures i by value and
     // j by reference.
     function<int (void)> f = [=, &j] { return i + j; };
  
     // Change the values of i and j.
     i = 22;
     j = 44;
  
     // Call f and print its result.
     std::cout << "f() = " << f() << endl;


	 // Call a lambda expression directly 
	 []{ cout << "Call me directly" << endl; }();

	 // Call the expression immediately with the arguments 5 and 4: 
	 int n = [] (int x, int y) { return x + y; }(5, 4);
     cout << "n = "<< n << endl;


	 // Create a list of integers with a few initial elements.
     list<int> numbers;
     numbers.push_back(13);
     numbers.push_back(17);
     numbers.push_back(42);
     numbers.push_back(46);
     numbers.push_back(99);
  
     // Use the find_if function and a lambda expression to find the 
     // first even number in the list.
     const list<int>::const_iterator result = find_if(numbers.begin(), numbers.end(), 
		 [](int n) { return (n % 2) == 0; });
  
     // Print the result.
     if (result != numbers.end())
     {
         cout << "The first even number in the list is " 
              << (*result) 
              << "." 
              << endl;
     }
     else
     {
         cout << "The list contains no even numbers." 
              << endl;
     }


	 // The following lambda expression contains a nested lambda
     // expression.
     int m = [](int x) { 
		 return [](int y) { return y * 2; }(x) + 3; 
	 }(5);
  
     // Print the result.
     cout << "m = " << m << endl;


	 // The following code declares a lambda expression that returns 
     // another lambda expression that adds two numbers. 
     // The returned lambda expression captures parameter x by value.
     auto g = [](int x) -> function<int (int)> 
        { return [=](int y) { return x + y; }; };
  
     // The following code declares a lambda expression that takes another
     // lambda expression as its argument.
     // The lambda expression applies the argument z to the function f
     // and adds 1.
     auto h = [](const function<int (int)>& f, int z) 
        { return f(z) + 1; };
  
     // Call the lambda expression that is bound to h. 
     auto a = h(g(7), 8);
  
     // Print the result.
     cout << "a = " << a << endl;


	 vector<int> values;
     values.push_back(1);
     values.push_back(10);
     values.push_back(100);
  
     // Create a Scale object that scales elements by 3 and apply
     // it to the vector object.
     Scale s(3);
     s.ApplyScale(values);


	 // Create a vector of integers with a few initial elements.
     vector<int> v;
     v.push_back(34);
     v.push_back(-43);
     v.push_back(56);
  
     // Negate each element in the vector.
     negate_all(v);
  
     // Print each element in the vector.
     print_all(v);



	return 0;
}