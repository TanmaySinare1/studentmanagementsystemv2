#ifndef AUTH_H
#define AUTH_H

#include <string>

bool isValidPassword(const std::string& password);
std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& storedValue);
bool login(const std::string& username, const std::string& password,
           int& userId, std::string& role, bool& mustChangePassword);
bool changePassword(int userId, const std::string& newPassword);

#endif
