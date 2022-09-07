#include <iostream>
#include <string>
#include <regex>

//  Types:
//
//  This is a template class that contains a regular expression
//	---------------------------------------------------------------------------
//	typedef basic_regex<char>		regex;
//	typedef basic_regex<wchar_t>	wregex;
//
//  A class that contains a sequence of matches.
//	match[0]: represents the entire match
//	match[1]: represents the first match (sub_match)
//	match[2]: represents the second match, and so forth
//	prefix(): returns the string that precedes the match
//	suffix(): returns the string that follows the match
//	---------------------------------------------------------------------------
//	typedef match_results<const char*>				cmatch;
//	typedef match_results<const wchar_t*>			wcmatch;
//	typedef match_results<string::const_iterator>	smatch;
//	typedef match_results<wstring::const_iterator>	wsmatch;
//
//	Represents a sequence of characters that match a capture group; an object 
//  of the match_results type can contain an array of objects of this type. 
//	---------------------------------------------------------------------------
//	typedef sub_match<const char*>				csub_match;
//	typedef sub_match<const wchar_t*>			wcsub_match;
//	typedef sub_match<string::const_iterator>	ssub_match;
//	typedef sub_match<wstring::const_iterator>	wssub_match;
//
//
//	Algorithms:
//
//	regex_match():		Completely matches a string with a regular expression, building 
//						sub-matches for the capture groups
//
//	regex_search():		Matches parts of a string with a regular expression, 
//						building sub-matches for the capture groups
//
//	regex_replace():	Replaces all the matches from a regular expression according 
//						to a specified format; optionally, you can replace only the first 
//						match or the parts of the string that did not produce a match
//
//	swap():				Swaps two objects of the basic_regex or match_result types
//
//
//	Iterators:
//
//	A forward constant iterator for iterating through all occurrences of 
//	a pattern in a string. There are several typedefs:
//	---------------------------------------------------------------------------
//	typedef regex_iterator<const char*>				cregex_iterator;
//	typedef regex_iterator<const wchar_t*>			wcregex_iterator;
//	typedef regex_iterator<string::const_iterator>	sregex_iterator;
//	typedef regex_iterator<wstring::const_iterator>	wsregex_iterator; 
//
//	A forwards constant iterator for iterating through the capture groups 
//	of all occurrences of a pattern in a string. Conceptually, it holds 
//	a regex_iterator object that it uses to search for regular expression 
//	matches in a character sequence.
//	---------------------------------------------------------------------------
//	typedef regex_token_iterator<const char*>				cregex_token_iterator;
//	typedef regex_token_iterator<const wchar_t*>			wcregex_token_iterator;
//	typedef regex_token_iterator<string::const_iterator>	sregex_token_iterator;
//	typedef regex_token_iterator<wstring::const_iterator>	wsregex_token_iterator;


bool is_email_valid(const std::string& email) {  

	// Define a regular expression   
	const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");

	// Try to match the string with the regular expression   
	return std::regex_match(email, pattern);
}

void show_ip_parts(const std::string& ip) {

	// Regular expression with 4 capture groups defined with parenthesis (...)   
	const std::regex pattern("(\\d{1,3}):(\\d{1,3}):(\\d{1,3}):(\\d{1,3})"); 

	// Object that will contain the sequence of sub-matches   
	std::match_results<std::string::const_iterator> result;

	// Match the IP address with the regular expression   
	bool valid = std::regex_match(ip, result, pattern);    
	std::cout << ip << " \t: " << (valid ? "valid" : "invalid") << std::endl; 

	// If the IP address matched the regex, then print the parts   
	if(valid)   {      
		std::cout << "b1: " << result[1] << std::endl;      
		std::cout << "b2: " << result[2] << std::endl;      
		std::cout << "b3: " << result[3] << std::endl;      
		std::cout << "b4: " << result[4] << std::endl;  
	}
}

std::string change_root(const std::string& item, const std::string& newroot) {   
	
	// Regular expression5.   
	const std::regex pattern("\\\\?((\\w|:)*)");

	// Transformation pattern   
	std::string replacer = newroot;  
	
	// Flag that indicates to transform only the first match    
	std::regex_constants::match_flag_type flag = std::regex_constants::format_first_only;

	// Apply the transformation15.   
	return std::regex_replace(item, pattern, replacer, flag);
}


std::string format_date(const std::string& date) {   
	
	// Regular expression   
	const std::regex pattern("(\\d{1,2})(\\.|-|/)(\\d{1,2})(\\.|-|/)(\\d{4})");

	// Transformation pattern, reverses the position of all capture groups   
	std::string replacer = "$5$4$3$2$1";

	// Apply the tranformation   
	return std::regex_replace(date, pattern, replacer);
}


int main(int argn, char* argv[])
{
	//
	// Matching
	// 
	std::string email1 = "marius.bancila@domain.com";   
	std::string email2 = "mariusbancila@domain.com";   
	std::string email3 = "marius_b@domain.co.uk";
	std::string email4 = "marius@domain";    
	std::cout << email1 << " : " << (is_email_valid(email1) ? "valid" : "invalid") << std::endl;   
	std::cout << email2 << " : " << (is_email_valid(email2) ? "valid" : "invalid") << std::endl;   
	std::cout << email3 << " : " << (is_email_valid(email3) ? "valid" : "invalid") << std::endl;  
	std::cout << email4 << " : " << (is_email_valid(email4) ? "valid" : "invalid") << std::endl;

	show_ip_parts("1:22:33:444");   
	show_ip_parts("1:22:33:4444");
	show_ip_parts("100:200");

	//
	// Searching
	//

	// Regular expression   
	const std::regex pattern("(\\w+day)");

	// The source text   
	std::string weekend = "Saturday and Sunday and Monday";

	// Sequence of string sub-matches
	std::cout << "regex_search()" << std::endl;
	std::smatch result; 
	bool match = std::regex_search(weekend, result, pattern);    
	if (match) {      
		// If there was a match print it
		for(size_t i = 1; i < result.size(); ++i) {         
			std::cout << result[i] << std::endl;   
		}   
	}

	std::cout << "sregex_token_iterator()" << std::endl;
	const std::sregex_token_iterator end; 
	for (std::sregex_token_iterator i(weekend.begin(), weekend.end(), pattern); i != end; ++i) {
		std::cout << *i << std::endl;   
	}

	std::string s = "Polygon: (1,2), (3,4), (5,6), (7,8)"; 
	const std::regex r("(\\d+),(\\d+)");       
	std::vector<int> v1;   
	v1.push_back(1);
	v1.push_back(2);    
	for (std::sregex_token_iterator i(s.begin(), s.end(), r, v1); i != end;) {
		int x = atoi((*i).str().c_str());
		++i;      
		int y = atoi((*i).str().c_str()); 
		++i;
		std::cout << "("<< x << ","<< y << ")"  << "\t";
	}
	std::cout << std::endl;

	std::vector<int> v2;   
	v2.push_back(1);    
	for (std::sregex_token_iterator i(s.begin(), s.end(), r, v2); i != end;) {
		int x = atoi((*i).str().c_str());
		++i;      
		int y = atoi((*i).str().c_str()); 
		++i;
		std::cout << "("<< x << ","<< y << ")"  << "\t";  
	}
	std::cout << std::endl;

	std::vector<int> v3;   
	v3.push_back(0);
	v3.push_back(1);
	v3.push_back(2);
	for (std::sregex_token_iterator i(s.begin(), s.end(), r, v3); i != end;) {
		std::cout << "entire match: " << (*i).str() << " sub match: ";	
		++i;
		std::cout << (*i).str() << "\t";								
		++i;  
		std::cout << (*i).str() << "\t";								
		++i;
		std::cout << std::endl;
	}

	//
	//	Transformations (or Replacing)
	//

	//	You can replace a match in a string according to a pattern. 
	//	This can either be a simple string, or a string representing a pattern 
	//	constructed with escape characters indicating capture groups.
	//  $1: What matches the first capture group
	//  $2: What matches the second capture group
	//	$&: What matches the whole regular expression
	//	$`: What appears before the whole regex
	//	$': What appears after the whole regex
	//	$$: $

	// Text to transform   
	std::string text = "This is a element and this a unique ID.";
	// Regular expression with two capture groups   
	const std::regex pat("(\\ba (a|e|i|u|o))+");      
	// The pattern for the transformation, using the second capture group   
	std::string replace = "an $2";    
	std::string newtext = std::regex_replace(text, pat, replace);
	std::cout << newtext << std::endl;

	std::string item1 = "\\dir\\dir2\\dir3";   
	std::string item2 = "c:\\folder\\";    
	std::cout << item1 << " -> " << change_root(item1, "\\dir1")  << std::endl;
	std::cout << item2 << " -> " << change_root(item2, "d:")      << std::endl;
	
	std::string date1 = "1/2/2008";   
	std::string date2 = "12.08.2008";    
	std::cout << date1 << " -> " << format_date(date1) << std::endl;
	std::cout << date2 << " -> " << format_date(date2) << std::endl;


	return 0;
}