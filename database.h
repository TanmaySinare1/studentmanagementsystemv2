#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

extern sqlite3* db;

bool openDatabase();
void closeDatabase();
void runSQL(const std::string& sql);
void setupDatabase();

#endif
