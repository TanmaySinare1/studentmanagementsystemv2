# Student Management System (C++ / SQLite)

A modular console-based Student Management System with Admin and Student roles.

## Features

- Admin and Student login
- Add and display student records
- Attendance management and attendance percentage
- Report students with marks below 35%
- Student fee setup and fee payment in multiple installments
- Installment payment history with timestamps
- Student self-service profile, attendance and fee views
- Password policy: 8-10 characters, at least one digit and one symbol
- Password storage using PBKDF2-HMAC-SHA256 with a fresh random salt per account

## Files

- `main.cpp`: application entry point and login flow
- `database.h/.cpp`: SQLite connection, schema and default accounts
- `auth.h/.cpp`: password policy, PBKDF2 hashing, verification and login
- `student.h/.cpp`: student account and profile functions
- `attendance.h/.cpp`: attendance operations
- `fees.h/.cpp`: total fees, installments and payment history
- `reports.h/.cpp`: below-35%-marks report
- `admin.h/.cpp`: Admin menu

## Requirements

- C++17-compatible `g++`
- SQLite development library
- OpenSSL development library (for PBKDF2 and secure random salts)

On a Debian/Ubuntu development machine, install dependencies only if you are authorized:

```bash
sudo apt update
sudo apt install g++ libsqlite3-dev libssl-dev
```

## Compile

Run from the folder containing all source files:

```bash
g++ -std=c++17 -g -O0 main.cpp database.cpp auth.cpp student.cpp attendance.cpp fees.cpp reports.cpp admin.cpp -lsqlite3 -lcrypto -o student_management
```

## Run

```bash
./student_management
```

The program creates `students.db` in the current working directory.

## Demo credentials for a newly created database

- Admin: `admin` / `Admin@123`
- Student: `student` / `Stud@1234`

Both passwords comply with the password policy. Change demonstration credentials before any real use.

### Existing database compatibility

The existing `users.password` column is reused to store a PBKDF2 encoded string. Existing plaintext accounts are not overwritten during database setup. On their next successful login, the application forces the user to choose a compliant password and saves its hash. Existing student, attendance and fee records are preserved.

Back up `students.db` before running an updated build against an existing database. Do not run migrations on a company or shared database without authorization.

## Password policy and storage

Passwords must contain 8-10 characters, at least one number and at least one symbol. The hash format stores the algorithm name, iteration count, random salt and derived hash in the `password` column. The application uses PBKDF2-HMAC-SHA256 (210,000 iterations) and a 16-byte random salt. Passwords are never stored as plain text for newly created or reset accounts.

## Fee workflow

1. Admin selects **Set / Update Total Student Fee** to set the total amount.
2. Admin selects **Add Fee Installment** each time the student makes a payment.
3. The program rejects an installment larger than the outstanding balance.
4. **View All Student Fees** shows total, paid and remaining amounts.
5. **View Fee Installment History** lists each payment and its timestamp.

The existing `fees` table keeps the total and cumulative paid amount. A new `fee_installments` table records individual payments. Existing paid totals are migrated as one opening-balance installment when no history exists.

## Important security notes

This is an educational project, not a production-ready payment or identity system. Use a proper password reset process, access controls, database backups and secure deployment practices for real use. The program does not encrypt the entire SQLite database or hide console password input.
