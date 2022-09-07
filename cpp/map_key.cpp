#include <map>
#include <unordered_map>
#include <iostream>

using namespace std;

struct KeyType {
	KeyType(int key) : x(key) {}
	
	// NOTES: "operator<" must be defined if this class is as the Key type of std::map, 
	//         Other defines like "operator==","operator!=", "operator()" are unncessary!
	friend bool operator < (const KeyType& k1, const KeyType& k2) {
		return k1.x < k2.x;
	}

	// NOTES: "operator ==" should be defined if  k1 == k2 is called
	friend bool operator == (const KeyType& k1, const KeyType& k2) {
		return k1.x == k2.x;
	}

	// NOTES: "operator !=" should be defined if  k1 != k2 is called
	friend bool operator != (const KeyType& k1, const KeyType& k2) {
		return k1.x != k2.x;
	}

	friend ostream& operator << (ostream& o, const KeyType& k) {
		o << "KeyType(" << k.x << ")";
		return o;
	}
private:
	int x = 0;
};


struct HashKeyType {
	HashKeyType(int key) : x(key) {}

	// NOTES: default constructor must be defined if this class as 3rd parameter of std::unordered_map
	HashKeyType() {}

	// NOTES: "operator<" must be defined if this class as 3rd parameter of std::unordered_map 
	std::size_t operator()(const HashKeyType& k) const noexcept {
		return std::hash<int>()(k.x);
	}

	// NOTES:
	// "operator ==" must be defined if insert()/find()/count()/... called or if k1 == k2 is called
	bool operator == (const HashKeyType& k) const {
		return x == k.x;
	}

	// NOTES:
	// "operator !=" should be defined if  k1 != k2 is called
	friend bool operator != (const HashKeyType& k1, const HashKeyType& k2) {
		return k1.x != k2.x;
	}

	friend ostream& operator << (ostream& o, const HashKeyType& k) {
		o << "HashKeyType(" << k.x << ")";
		return o;
	}
private:
	int x = 0;
};

// NOTES: optionally, implement specialization of std::hash, then we can write code like the following 
//        unordered_map<HashKeyType, int> m;
template<>
struct std::hash<HashKeyType> {
	std::size_t operator()(const HashKeyType& k) const noexcept {
		return k.operator()(k);
	}
};


void testKeyType() {
	map<KeyType, int> m;
	KeyType k1(1);
	KeyType k2(2);

	m[k1] = 10;
	m[k2] = 20;

	auto r1 = m.emplace(k2, 30);
	if (!r1.second)
		cout << "emplace key=" << k2 << " failed" << endl;

	auto r2 = m.find(KeyType(2));
	if (r2 != m.end())
		cout << "found key=" << r2->first << " value=" << r2->second << endl;
	cout << "count of " << KeyType(2) << " is " << m.count(KeyType(2)) << endl;

	cout << k1 << "==" << k2 << " is " << (k1 == k2) << endl;
	cout << k1 << "!=" << k2 << " is " << (k1 != k2) << endl;
}


void testHashKeyType() {
	// NOTES: equal to "unordered_map<HashKeyType, int> m;" if std::hash<HashKeyType> is defined as above
	unordered_map<HashKeyType, int, HashKeyType> m;
	HashKeyType k1(10);
	HashKeyType k2(20);
	m[k1] = 100;
	m[k2] = 200;

	auto r1 = m.emplace(k2, 30);
	if (!r1.second)
		cout << "emplace key=" << k2 << " failed" << endl;

	auto r2 = m.find(HashKeyType(20));
	if (r2 != m.end())
		cout << "found key=" << r2->first << " value=" << r2->second << endl;
	cout << "count of " << HashKeyType(20) << " is " << m.count(HashKeyType(2)) << endl;

	cout << k1 << "==" << k2 << " is " << (k1 == k2) << endl;
	cout << k1 << "!=" << k2 << " is " << (k1 != k2) << endl;
}


int main(int argc, char* argv[]) {
	cout << " ----------------test KeyType ----------------" << endl;
	testKeyType();
	cout << " ----------------test HashKeyType ----------------" << endl;
	testHashKeyType();
	return 0;
}