#include <iostream>
#include <boost/lexical_cast.hpp>



struct CX {
    int x;
};

struct CY {
    std::string y;
};


std::ostream& operator << (std::ostream& os, const CX& cx) {
    os << cx.x;
    return  os;
}

std::istream& operator >> (std::istream& is, CY& cy) {
    is >> cy.y;
    return  is;
}


int main(int argc, char** argv)
{
    int x = boost::lexical_cast<int>("100");
    std::string y = boost::lexical_cast<std::string>(x);
    std::cout << "x=" << x << std::endl;
    std::cout << "y=" << y << std::endl;

    CX cx = {200};
    CY cy = boost::lexical_cast<CY>(cx);

    std::cout << "cx.x=" << cx.x << std::endl; 
    std::cout << "cy.y=" << cy.y << std::endl;   

    return 0;
}
