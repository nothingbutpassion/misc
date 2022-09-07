#include <string>
#include <iostream>
#include <boost/regex.hpp>


inline std::string regexMatch(const std::string& src, const std::string& regex, std::size_t group = 0) {
    boost::smatch m;
    bool find = boost::regex_match(src.begin(), src.end(), m, boost::regex(regex));
    if (find && group < m.size()) {
       return m.str(group); 
    }
    return "";   
}

std::string regexReplace(const std::string& src, const std::string& rgx, const std::string& fmt) {
    boost::regex r(rgx);
    return boost::regex_replace(src, r, fmt);   
}

bool regexSearch(const std::string& src, const std::string& rgx, std::string& matched) {
    boost::regex r(rgx);
    boost::smatch m;
    bool find = boost::regex_search(src.begin(), src.end(), m, r);
    if (find) {
        matched = m.str(0);
        for (int i=0; i < m.size(); i++) {
            std::cout << i << ":" << m.str(i) << std::endl;
        }
    }
    return find;   
}

std::string regexSearch(const std::string& src, const std::string& rgx, size_t matchIndex) {
    boost::regex r(rgx);
    boost::smatch m;
    bool find = boost::regex_search(src.begin(), src.end(), m, r);
    if (find && matchIndex < m.size()) {
       return m.str(matchIndex); 
    }
    return "";   
}


int main (int argc, char** argv) {

    // replace
    std::string src = "there is a subsequence in the string";
    boost::regex rex("\\b(sub)([^ ]*)");
    std::string replace = "\\1 \\2";
    std::cout << boost::regex_replace(src, rex, replace) << std::endl;

    // search
    std::string input = "1234test1, Test2 X test3 Y test4Z";
    boost::regex r("(test[0-9])([, X-Y]+)");
    boost::smatch sm;
    std::string::const_iterator start = input.begin();
    std::string::const_iterator end   = input.end();
    while (boost::regex_search(start, end, sm, r)) {
        std::cout << "prefix=" << sm.prefix() << std::endl;
        for(int i=0; i < sm.size(); i++) {
            std::cout << "sub_match=" << i
                      << " position:" << sm.position(i) 
                      << " length:" << sm.length(i) 
                      << " str:" << sm.str(i)
                      << std::endl;       
        }
        std::cout << "suffix=" << sm.suffix() << std::endl;
        std::cout << std::endl;
        start = sm[0].second;
   }


   // replace wrapper
   std::cout << regexReplace("{ \"resource\": \"/tasks/29\" }", "(\"resource\")\\s*:\\s*(\"/tasks(/.*)*\")", "\\1=\\2") << std::endl;
   std::cout << std::endl;
    
   // search wrapper
   std::string out;
   std::string resource = "\"(resource)\"\\s*:\\s*\"/(facts|tasks|components|status|settings|updates|sensors)(/.*)*\"";
   std::string method = "\"(method)\"\\s*:\\s*\"(GET|PUT|POST|DEL)\"";
   regexSearch("{ \n\"resource\":\n\"/tasks/29/settings\"\n }", resource, out);

   std::cout << regexSearch("{ \n\"resource\"\t:\t\"/components/100\"\n}", resource, 2) << std::endl;
   std::cout << regexSearch("{ \n\"resource\":\"/status\"\n}", resource, 2) << std::endl;

   std::cout << regexSearch("{ \n\"method\":\"GET\"\n}", method, 2) << std::endl;
   std::cout << regexSearch("{ \n\"method\":\"POST\"\n}", method, 2) << std::endl;

   regexSearch("{ \"method\":\"DEL\" }", method, out);


   std::cout << regexMatch("/tasks/29", "/(facts|tasks|components|status|settings|updates|sensors)(/.+)*") << std::endl;
   return 0;
}
