#include "student.h"
#include "database.h"
#include "auth.h"
#include <iostream>
#include <string>
using namespace std;

int getStudentId(int userId)
{
    sqlite3_stmt* stmt = nullptr;
    int studentId = -1;

    sqlite3_prepare_v2(db, "SELECT id FROM students WHERE user_id=?",
                       -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, userId);

    if (sqlite3_step(stmt) == SQLITE_ROW)
        studentId = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return studentId;
}

void addStudent()
{
    string username, password, name, course;
    double marks;

    cout << "New student username: ";
    cin >> username;

    do
    {
        cout << "New student password (8-10 chars, at least 1 number and 1 symbol): ";
        cin >> password;
        if (!isValidPassword(password))
            cout << "Invalid password. Example format: Sai@1234\n";
    } while (!isValidPassword(password));

    string encoded = hashPassword(password);
    if (encoded.empty())
    {
        cout << "Could not securely hash password. Student was not added.\n";
        return;
    }

    cin.ignore(10000, '\n');
    cout << "Student name: ";
    getline(cin, name);
    cout << "Course: ";
    getline(cin, course);
    cout << "Marks percentage: ";
    cin >> marks;

    if (marks < 0 || marks > 100)
    {
        cout << "Marks must be between 0 and 100.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* userSQL =
        "INSERT INTO users(username,password,role) VALUES(?,?,'student')";

    if (sqlite3_prepare_v2(db, userSQL, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cout << "Could not prepare account creation.\n";
        return;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, encoded.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        cout << "Could not add account. Username may already exist.\n";
        sqlite3_finalize(stmt);
        return;
    }

    int userId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);

    const char* studentSQL =
        "INSERT INTO students(user_id,name,course,marks) VALUES(?,?,?,?)";
    sqlite3_prepare_v2(db, studentSQL, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, course.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, marks);

    if (sqlite3_step(stmt) == SQLITE_DONE)
    {
        int studentId = static_cast<int>(sqlite3_last_insert_rowid(db));
        sqlite3_finalize(stmt);

        sqlite3_prepare_v2(db, "INSERT INTO attendance(student_id) VALUES(?)",
                           -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, studentId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        sqlite3_prepare_v2(db, "INSERT INTO fees(student_id) VALUES(?)",
                           -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, studentId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        cout << "Student added successfully. Student ID: " << studentId << '\n';
    }
    else
    {
        cout << "Could not save student profile.\n";
        sqlite3_finalize(stmt);
    }
}

void displayStudents()
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.id,u.username,s.name,s.course,s.marks "
        "FROM students s JOIN users u ON s.user_id=u.id ORDER BY s.id";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n--- ALL STUDENTS ---\n";

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        cout << "\nID: " << sqlite3_column_int(stmt, 0)
             << "\nUsername: " << sqlite3_column_text(stmt, 1)
             << "\nName: " << sqlite3_column_text(stmt, 2)
             << "\nCourse: " << sqlite3_column_text(stmt, 3)
             << "\nMarks: " << sqlite3_column_double(stmt, 4) << "%\n";
    }

    sqlite3_finalize(stmt);
}

void viewStudentProfile(int studentId)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
        "SELECT name,course,marks FROM students WHERE id=?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, studentId);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        cout << "\n--- MY PROFILE ---\n"
             << "Name: " << sqlite3_column_text(stmt, 0)
             << "\nCourse: " << sqlite3_column_text(stmt, 1)
             << "\nMarks: " << sqlite3_column_double(stmt, 2) << "%\n";
    }
    else
        cout << "Student profile not found.\n";

    sqlite3_finalize(stmt);
}
