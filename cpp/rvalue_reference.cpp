#include <iostream>
#include <string>

using namespace std;
 


// This example illustrates how rvalue references support the implementation of 
// "Move Semantics" and "Pperfect Forwarding".
//

// A class that contains a memory resource.
class MemoryBlock
{
    // TODO: Add resources for the class here.
};
  
void g(const MemoryBlock&) 
{
    cout << "In g(const MemoryBlock&)." << endl;
}

  
void g(MemoryBlock&&) 
{
    cout << "In g(MemoryBlock&&)." << endl;
}

MemoryBlock&& f(MemoryBlock&& block)
{
    g(block);

	// This template function std::move(Type&& t) returns t as an rvalue reference, 
	// whether or not Type is a reference type
	return std::move(block);
	// Equal expressions:
	//return static_cast<MemoryBlock&&>(block);
	//return (MemoryBlock&&)block;
	
}


template<typename T> struct S;
  
// The following structures specialize S by 
// lvalue reference (T&), const lvalue reference (const T&), 
// rvalue reference (T&&), and const rvalue reference (const T&&).
// Each structure provides a print method that prints the type of 
// the structure and its parameter.
  
template<typename T> struct S<T&> {
    static void print(T& t)
    {
    cout << "print<T&>: " << t << endl;
    }
};
  
template<typename T> struct S<const T&> {
    static void print(const T& t)
    {
    cout << "print<const T&>: " << t << endl;
    }
};
  
template<typename T> struct S<T&&> {
    static void print(T&& t)
    {
    cout << "print<T&&>: " << t << endl;
    }
};
  
template<typename T> struct S<const T&&> {
    static void print(const T&& t)
    {
    cout << "print<const T&&>: " << t << endl;
    }
};

// Function templates deduce their template argument types and then use 
// reference collapsing rules.The following table summarizes the reference 
// collapsing rules for template argument type deduction:
// ---------------------------------------
// Expanded type		Collapsed type
// ---------------------------------------
// T&	&				T& 
// T&	&&				T&
// T&&	&				T&
// T&&	&&				T&&
// ---------------------------------------
// This function forwards its parameter to a specialized
// version of the S type.
template <typename T> void print_type_and_value(T&& t) 
{
	// std::forward<T>(t) returns an rvalue reference if t is an rvalue, 
	// or an lvalue reference if t is an lvalue. 
    S<T&&>::print(std::forward<T>(t));
	// Equale expressions :
	//S<T&&>::print(static_cast<T&&>(t));
	//S<T&&>::print((T&&)t);
}
  
// This function returns the constant string "fourth".
const string fourth() { return string("fourth"); }


int main(int argc, char* argv[])
{
	MemoryBlock block;
	g(block);
	g(MemoryBlock());
	g(f(MemoryBlock()));


	// The following call resolves to:
	// print_type_and_value<string&>(string& && t)
	// Which collapses to:
	// print_type_and_value<string&>(string& t)
	string s1("first");
	print_type_and_value(s1); 
  
	// The following call resolves to:
	// print_type_and_value<const string&>(const string& && t)
	// Which collapses to:
	// print_type_and_value<const string&>(const string& t)
	const string s2("second");
	print_type_and_value(s2);
  
	// The following call resolves to:
	// print_type_and_value<string&&>(string&& t)
	print_type_and_value(string("third"));
  
	// The following call resolves to:
	// print_type_and_value<const string&&>(const string&& t)
	print_type_and_value(fourth());

	return 0;
}