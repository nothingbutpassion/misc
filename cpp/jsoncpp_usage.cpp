#include <iostream>
#include <sstream>
#include "jsoncpp.hpp"

using namespace std;

int main(int argc, char** argv) {


    try {
        //
        // encode
        //
        Json::Value root;
        root["id"] = 100;
        root["name"]="bird";
        root["age"] = 1.0;
        root["hobby"][0] = "reading";
        root["hobby"][1] = "music";

        // will throw exception
        // root[0] = 200; 


        //stringstream ss;
        //ss << root;
        //cout << ss.str() << endl;

        Json::StyledWriter writer;
        string json = writer.write(root);
        cout << json << endl;

        

        //
        // decode
        //
        Json::Value root2;
        
        // ss >> root2;

        Json::Reader reader;
        reader.parse(json, root2);
        

        int id = root2["id"].asInt();
        cout << "id=" << id << endl;

        string name = root2["id"].asString();
        cout << "name=" << name << endl;

        double age = root2["id"].asDouble();
        cout << "age=" << age << endl;
        
        Json::Value hobby = root["hobby"];
        cout << "hobby=";
        for (int i=0; i < hobby.size(); i++) {
            cout << hobby[i].asString() << " ";
    
        }
        cout << endl;        
        



    } catch (const Json::Exception& e) {
        cout << e.what();
    }

    

    
    return 0;
}
