// extract to string
#include <iostream>
#include <string>
#include <stdio.h>
#include <sstream>

using namespace std;

int main ()
{
/*
  std::string name;
  std::string id;

  std::cout << "Please, enter your full name: ";
  std::getline (std::cin,name);

  std::cout << "Please, enter your id: ";
  std::getline (std::cin,id);

  std::cout << "Hello, " << name << "!\n";
  std::cout << "Hello, " << id << "!\n";


  char address [256];
  printf ("Insert your full address: ");

  fgets(address, sizeof(address), stdin);
  printf ("gets: %s\n", address);
  }

  while(fgets(address, sizeof(address), stdin)) {
       printf ("gets: %s\n", address);
  }


  std::string str1 = "hello";
  str1 += ' ';
  str1 += "world";

  string str2 = "ok";

  std::cout << str1 + str2 << std::endl;

  */ 
    istringstream iss("one two \t three\t\r\nfour \r\n five \n");
    string sub; 
    while (iss >> sub) {
      cout << "substring: " << sub << endl;
    }

    stringstream ss;
    int a;
    string b;
    ss << 100;
    ss << 2; 
    ss << " \n\t";
    ss << 1000;   
    ss >> a;
    ss >> b;
    cout << a << "+" << b <<endl;
    
    char s[4];
    int r = snprintf (s, sizeof(s), "hello, world");
    cout << r << endl;
    
    return 0;
}
