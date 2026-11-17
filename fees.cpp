#include "fees.h"
#include "database.h"
#include <iostream>
#include <iomanip>
using namespace std;

// Sets the total fee. It never overwrites the amount already paid.
void updateFee()
{
    int id;
    double totalFee;

    cout << "Student ID: ";
    cin >> id;
    cout << "Total course fee: ";
    cin >> totalFee;

    if (id <= 0 || totalFee < 0)
    {
        cout << "Invalid student ID or fee amount.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE fees SET total_fee=? WHERE student_id=? AND paid_fee <= ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cout << "Could not prepare fee update.\n";
        return;
    }
    sqlite3_bind_double(stmt, 1, totalFee);
    sqlite3_bind_int(stmt, 2, id);
    sqlite3_bind_double(stmt, 3, totalFee);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) == 1;
    sqlite3_finalize(stmt);

    if (ok)
        cout << "Total fee updated. Existing installments are unchanged.\n";
    else
        cout << "Update failed: check Student ID and ensure total fee is not below amount already paid.\n";
}

// Adds a payment installment and updates paid_fee in one transaction.
void addFeeInstallment()
{
    int id;
    double amount;

    cout << "Student ID: ";
    cin >> id;
    cout << "Installment amount: ";
    cin >> amount;

    if (id <= 0 || amount <= 0)
    {
        cout << "Student ID and installment must be greater than zero.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT total_fee,paid_fee FROM fees WHERE student_id=?",
        -1, &stmt, nullptr) != SQLITE_OK)
    {
        cout << "Could not read fee details.\n";
        return;
    }
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        cout << "Student fee record not found.\n";
        sqlite3_finalize(stmt);
        return;
    }

    double total = sqlite3_column_double(stmt, 0);
    double paid = sqlite3_column_double(stmt, 1);
    sqlite3_finalize(stmt);

    if (amount > total - paid + 0.000001)
    {
        cout << "Installment exceeds remaining balance. Balance: "
             << fixed << setprecision(2) << (total - paid) << '\n';
        return;
    }

    if (sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        cout << "Could not start payment transaction.\n";
        return;
    }

    bool ok = false;
    if (sqlite3_prepare_v2(db,
        "INSERT INTO fee_installments(student_id,amount) VALUES(?,?)",
        -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_double(stmt, 2, amount);
        ok = sqlite3_step(stmt) == SQLITE_DONE;
    }
    sqlite3_finalize(stmt);
    stmt = nullptr;

    if (ok && sqlite3_prepare_v2(db,
        "UPDATE fees SET paid_fee=paid_fee+? WHERE student_id=? AND paid_fee+? <= total_fee+0.000001",
        -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_double(stmt, 1, amount);
        sqlite3_bind_int(stmt, 2, id);
        sqlite3_bind_double(stmt, 3, amount);
        ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) == 1;
    }
    else
        ok = false;

    sqlite3_finalize(stmt);

    if (ok)
    {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        cout << "Installment recorded successfully.\n";
    }
    else
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        cout << "Payment could not be recorded. No changes were saved.\n";
    }
}

void viewStudentFee(int studentId)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
        "SELECT total_fee,paid_fee FROM fees WHERE student_id=?",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, studentId);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double total = sqlite3_column_double(stmt, 0);
        double paid = sqlite3_column_double(stmt, 1);
        double balance = total - paid;

        cout << fixed << setprecision(2)
             << "\n--- FEE DETAILS ---\n"
             << "Total fee: " << total
             << "\nPaid fee: " << paid
             << "\nBalance: " << balance
             << "\nStatus: " << (balance <= 0.009 ? "NIL" : "PENDING") << '\n';
    }
    else
        cout << "Fee details not found.\n";

    sqlite3_finalize(stmt);
}

void viewAllFees()
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT s.id,s.name,COALESCE(f.total_fee,0),COALESCE(f.paid_fee,0) "
        "FROM students s LEFT JOIN fees f ON s.id=f.student_id ORDER BY s.id";

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    cout << "\n--- ALL STUDENT FEES ---\n";

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double total = sqlite3_column_double(stmt, 2);
        double paid = sqlite3_column_double(stmt, 3);
        double balance = total - paid;

        cout << fixed << setprecision(2)
             << "\nID: " << sqlite3_column_int(stmt, 0)
             << "\nName: " << sqlite3_column_text(stmt, 1)
             << "\nTotal fee: " << total
             << "\nPaid fee: " << paid
             << "\nBalance: " << balance
             << "\nStatus: " << (balance <= 0.009 ? "NIL" : "PENDING") << '\n';
    }

    sqlite3_finalize(stmt);
}

void viewFeeInstallments()
{
    int id;
    cout << "Student ID: ";
    cin >> id;
    if (id <= 0)
    {
        cout << "Invalid Student ID.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT id,amount,payment_date FROM fee_installments "
        "WHERE student_id=? ORDER BY id";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        cout << "Could not read installment history.\n";
        return;
    }
    sqlite3_bind_int(stmt, 1, id);

    cout << "\n--- FEE INSTALLMENT HISTORY ---\n";
    bool found = false;
    double sum = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        found = true;
        double amount = sqlite3_column_double(stmt, 1);
        sum += amount;
        const unsigned char* date = sqlite3_column_text(stmt, 2);

        cout << "Payment ID: " << sqlite3_column_int(stmt, 0)
             << " | Amount: " << fixed << setprecision(2) << amount
             << " | Date: " << (date ? reinterpret_cast<const char*>(date) : "")
             << '\n';
    }

    if (!found) cout << "No installment payments recorded.\n";
    cout << "Total installments recorded: " << fixed << setprecision(2) << sum << '\n';
    sqlite3_finalize(stmt);
}
