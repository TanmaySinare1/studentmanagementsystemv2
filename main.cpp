#include <iostream>
#include <string>
#include "database.h"
#include "auth.h"
#include "admin.h"
#include "student.h"
#include "attendance.h"
#include "fees.h"
using namespace std;

void studentMenu(int userId)
{
    int studentId = getStudentId(userId);

    if (studentId == -1)
    {
        cout << "Student profile not found.\n";
        return;
    }

    int choice;
    do
    {
        cout << "\n========== STUDENT MENU ==========\n"
             << "1. View My Profile\n"
             << "2. View My Attendance\n"
             << "3. View My Fee Details\n"
             << "0. Logout\n"
             << "Enter choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1: viewStudentProfile(studentId); break;
            case 2: viewStudentAttendance(studentId); break;
            case 3: viewStudentFee(studentId); break;
            case 0: cout << "Student logged out.\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 0);
}

int main()
{
    if (!openDatabase())
        return 1;

    setupDatabase();

    int choice;
    do
    {
        cout << "\n===== STUDENT MANAGEMENT SYSTEM =====\n"
             << "1. Admin Login\n"
             << "2. Student Login\n"
             << "0. Exit\n"
             << "Enter choice: ";
        cin >> choice;

        if (choice == 1 || choice == 2)
        {
            string username, password, role;
            int userId = 0;
            bool mustChangePassword = false;

            cout << "Username: ";
            cin >> username;
            cout << "Password: ";
            cin >> password;

            if (login(username, password, userId, role, mustChangePassword))
            {
                if (mustChangePassword)
                {
                    cout << "\nYour account uses an old or weak password.\n"
                         << "Set a new password (8-10 characters, at least one number and one symbol).\n";

                    string newPassword;
                    do
                    {
                        cout << "New password: ";
                        cin >> newPassword;
                        if (!isValidPassword(newPassword))
                            cout << "Invalid password. Example format: Admin@123\n";
                    } while (!isValidPassword(newPassword));

                    if (!changePassword(userId, newPassword))
                    {
                        cout << "Password update failed. Login cancelled.\n";
                        continue;
                    }
                    cout << "Password updated and securely hashed.\n";
                }

                if (choice == 1 && role == "admin")
                    adminMenu();
                else if (choice == 2 && role == "student")
                    studentMenu(userId);
                else
                    cout << "This account cannot use that login type.\n";
            }
            else
                cout << "Invalid username or password.\n";
        }
        else if (choice != 0)
            cout << "Invalid choice.\n";

    } while (choice != 0);

    closeDatabase();
    cout << "Program ended.\n";
    return 0;
}
