#include "reports.h"
#include "database.h"
#include <iostream>
using namespace std;

void viewFailedStudents()
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id,name,course,marks FROM students WHERE marks < 35 ORDER BY marks";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n--- STUDENTS BELOW 35% MARKS ---\n";

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        cout << "\nStudent ID: " << sqlite3_column_int(stmt, 0)
             << "\nName: " << sqlite3_column_text(stmt, 1)
             << "\nCourse: " << sqlite3_column_text(stmt, 2)
             << "\nMarks: " << sqlite3_column_double(stmt, 3) << "%\n";
    }

    if (!found)
        cout << "No students below 35%.\n";

    sqlite3_finalize(stmt);
}
