#include "admin.h"
#include "student.h"
#include "attendance.h"
#include "fees.h"
#include "reports.h"
#include <iostream>
using namespace std;

void adminMenu()
{
    int choice;

    do
    {
        cout << "\n========== ADMIN MENU ==========\n"
             << "1. Add Student\n"
             << "2. Display All Students\n"
             << "3. Update Attendance\n"
             << "4. View All Attendance\n"
             << "5. View Students Below 35% Marks\n"
             << "6. Set / Update Total Student Fee\n"
             << "7. Add Fee Installment\n"
             << "8. View All Student Fees\n"
             << "9. View Fee Installment History\n"
             << "0. Logout\n"
             << "Enter choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1: addStudent(); break;
            case 2: displayStudents(); break;
            case 3: updateAttendance(); break;
            case 4: viewAllAttendance(); break;
            case 5: viewFailedStudents(); break;
            case 6: updateFee(); break;
            case 7: addFeeInstallment(); break;
            case 8: viewAllFees(); break;
            case 9: viewFeeInstallments(); break;
            case 0: cout << "Admin logged out.\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 0);
}
