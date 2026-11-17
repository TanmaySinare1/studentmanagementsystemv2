#include "attendance.h"
#include "database.h"
#include <iostream>
using namespace std;

void updateAttendance()
{
    int id, attended, total;

    cout << "Student ID: ";
    cin >> id;
    cout << "Total classes held: ";
    cin >> total;
    cout << "Classes attended: ";
    cin >> attended;

    if (id <= 0 || total < 0 || attended < 0 || attended > total)
    {
        cout << "Invalid attendance values. Attended cannot exceed total.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO attendance(student_id,attended,total) VALUES(?,?,?) "
        "ON CONFLICT(student_id) DO UPDATE SET "
        "attended=excluded.attended,total=excluded.total";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, attended);
    sqlite3_bind_int(stmt, 3, total);

    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0)
        cout << "Attendance updated successfully.\n";
    else
        cout << "Student ID not found or update failed.\n";

    sqlite3_finalize(stmt);
}

void viewAllAttendance()
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.id,s.name,COALESCE(a.attended,0),COALESCE(a.total,0) "
        "FROM students s LEFT JOIN attendance a ON s.id=a.student_id ORDER BY s.id";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n--- ALL STUDENT ATTENDANCE ---\n";

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int attended = sqlite3_column_int(stmt, 2);
        int total = sqlite3_column_int(stmt, 3);
        double percentage = total > 0 ? attended * 100.0 / total : 0;

        cout << "\nID: " << sqlite3_column_int(stmt, 0)
             << "\nName: " << sqlite3_column_text(stmt, 1)
             << "\nClasses attended: " << attended
             << "\nTotal classes: " << total
             << "\nAttendance: " << percentage << "%\n";
    }

    sqlite3_finalize(stmt);
}

void viewStudentAttendance(int studentId)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
        "SELECT attended,total FROM attendance WHERE student_id=?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, studentId);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int attended = sqlite3_column_int(stmt, 0);
        int total = sqlite3_column_int(stmt, 1);
        double percentage = total > 0 ? attended * 100.0 / total : 0;

        cout << "\n--- MY ATTENDANCE ---\n"
             << "Classes attended: " << attended
             << "\nTotal classes: " << total
             << "\nAttendance percentage: " << percentage << "%\n";
    }
    else
        cout << "Attendance details not available.\n";

    sqlite3_finalize(stmt);
}
