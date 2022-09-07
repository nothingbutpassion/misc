#include <iostream>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


using namespace std;
using namespace boost;

/*
{
    "id": "123",
    "name": "abc",
    "age": 18
    "love":
    [
        "book",
        "music"
    ]
}
*/


int main(int argc, char** argv) {

    property_tree::ptree pt0;
    property_tree::ptree pt1;
    property_tree::ptree pt2;

    // construct ptree object
    pt0.put("id", 123);
    pt0.add("name", "abc");

    pt2.put_value(18);
    pt0.add_child("age", pt2);

    pt1.push_back(make_pair("", "book"));
    pt1.push_back(make_pair("", "music"));
    pt0.add_child("love", pt1);

	stringstream ss;
	property_tree::write_json(ss, pt0);
	cout << ss.str() << endl;

    // access ptree object
    int id = pt0.get<int>("id");
    string name = pt0.get<string>("name");
    int age = pt0.get_child("age").get_value<int>();

    property_tree::ptree ptc = pt0.get_child("love");

    
    cout << "id: " << id << endl;
    cout << "name: " << name << endl;
    cout << "age: " << age << endl; 
    cout << "love: \n[\n";

    property_tree::ptree::iterator it;
    for (it = ptc.begin(); it != ptc.end(); ++it) {
        cout << "\t" <<it->second.data() << endl;
    } 
    cout << "]\n";

    return 0;
}
