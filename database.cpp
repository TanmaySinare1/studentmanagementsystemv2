#include "database.h"
#include "auth.h"
#include <iostream>

sqlite3* db = nullptr;

static void insertDefaultAccountIfMissing(const char* username,
                                          const char* plainPassword,
                                          const char* role)
{
    sqlite3_stmt* check = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM users WHERE username=?", -1,
                           &check, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_text(check, 1, username, -1, SQLITE_STATIC);
    bool exists = sqlite3_step(check) == SQLITE_ROW;
    sqlite3_finalize(check);
    if (exists) return;

    std::string encoded = hashPassword(plainPassword);
    if (encoded.empty()) return;

    sqlite3_stmt* insert = nullptr;
    if (sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO users(username,password,role) VALUES(?,?,?)",
        -1, &insert, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(insert, 1, username, -1, SQLITE_STATIC);
        sqlite3_bind_text(insert, 2, encoded.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert, 3, role, -1, SQLITE_STATIC);
        sqlite3_step(insert);
    }
    sqlite3_finalize(insert);
}

void runSQL(const std::string& sql)
{
    char* error = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK)
    {
        std::cout << "Database error: " << (error ? error : "unknown") << '\n';
        sqlite3_free(error);
    }
}

bool openDatabase()
{
    if (sqlite3_open("students.db", &db) != SQLITE_OK)
    {
        std::cout << "Cannot open database.\n";
        return false;
    }
    return true;
}

void closeDatabase()
{
    if (db != nullptr)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

void setupDatabase()
{
    runSQL("PRAGMA foreign_keys=ON;");

    // Keep the existing password column for compatibility. It now stores
    // a PBKDF2-SHA256 encoded value rather than the original password.
    runSQL("CREATE TABLE IF NOT EXISTS users("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "username TEXT UNIQUE NOT NULL,"
           "password TEXT NOT NULL,"
           "role TEXT NOT NULL);");

    runSQL("CREATE TABLE IF NOT EXISTS students("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "user_id INTEGER UNIQUE NOT NULL,"
           "name TEXT NOT NULL,"
           "course TEXT NOT NULL,"
           "marks REAL DEFAULT 0,"
           "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE);");

    runSQL("CREATE TABLE IF NOT EXISTS attendance("
           "student_id INTEGER PRIMARY KEY,"
           "attended INTEGER DEFAULT 0,"
           "total INTEGER DEFAULT 0,"
           "FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE);");

    runSQL("CREATE TABLE IF NOT EXISTS fees("
           "student_id INTEGER PRIMARY KEY,"
           "total_fee REAL DEFAULT 0,"
           "paid_fee REAL DEFAULT 0,"
           "FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE);");

    runSQL("CREATE TABLE IF NOT EXISTS fee_installments("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "student_id INTEGER NOT NULL,"
           "amount REAL NOT NULL CHECK(amount > 0),"
           "payment_date TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY(student_id) REFERENCES students(id) ON DELETE CASCADE);");

    // Seed secure demo accounts only when the usernames do not already exist.
    // Existing legacy accounts are not overwritten; they are asked to reset
    // their password after a successful legacy login.
    insertDefaultAccountIfMissing("admin", "Admin@123", "admin");
    insertDefaultAccountIfMissing("student", "Stud@1234", "student");

    runSQL("INSERT OR IGNORE INTO students(user_id,name,course,marks) "
           "SELECT id,'Demo Student','Computer Engineering',85 "
           "FROM users WHERE username='student';");

    runSQL("INSERT OR IGNORE INTO attendance(student_id) SELECT id FROM students;");
    runSQL("INSERT OR IGNORE INTO fees(student_id) SELECT id FROM students;");

    // Preserve pre-existing paid_fee totals by recording them as an opening
    // balance installment once, so subsequent payments have a history.
    runSQL("INSERT INTO fee_installments(student_id,amount) "
           "SELECT f.student_id,f.paid_fee FROM fees f "
           "WHERE f.paid_fee > 0 AND NOT EXISTS "
           "(SELECT 1 FROM fee_installments i WHERE i.student_id=f.student_id);");
}
