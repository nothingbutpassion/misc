#include <sqlite3.h>
#include <iostream>

using namespace std;


// NOTES:
// If an sqlite3_exec() callback returns non-zero, the sqlite3_exec() routine returns SQLITE_ABORT without invoking 
// the callback again and without running any subsequent SQL statements.
static int callback(void* custom, int cols, char** colValues, char** colNames) {

    cout << "callback: " << endl;

    for (int i=0; i < cols; i++) {
        cout << colNames[i] << "=" << (colValues[i] ? colValues[i] : "null") << endl;   
    }

    return 0;
}


int main(int argc, char** argv) {

    if (argc != 3) {
        cerr << "usage: " << argv[0] << " <db-file> <sql-commands>" << endl;
        return -1;
    }

    // create or open sqlite3 (call sqlite3 constructor)
    sqlite3* db = NULL;
    if (SQLITE_OK != sqlite3_open(argv[1], &db)) {
        cerr << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return -1;
    }  

    char* errmsg = NULL;
    if (SQLITE_OK != sqlite3_exec(db, argv[2], callback, NULL, &errmsg)) {
        sqlite3_free(errmsg);
    }

    // call sqlite3 destructor
    sqlite3_close(db);
    return 0;
}
