#include "auth.h"
#include "database.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <iomanip>
#include <sstream>
#include <vector>

namespace
{
const int ITERATIONS = 210000;
const int SALT_SIZE = 16;
const int HASH_SIZE = 32;

std::string toHex(const unsigned char* data, size_t length)
{
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i)
        out << std::setw(2) << static_cast<int>(data[i]);
    return out.str();
}

bool fromHex(const std::string& input, std::vector<unsigned char>& output)
{
    if (input.size() % 2 != 0) return false;
    output.clear();
    for (size_t i = 0; i < input.size(); i += 2)
    {
        unsigned int value = 0;
        std::istringstream in(input.substr(i, 2));
        in >> std::hex >> value;
        if (in.fail()) return false;
        output.push_back(static_cast<unsigned char>(value));
    }
    return true;
}
}

bool isValidPassword(const std::string& password)
{
    if (password.size() < 8 || password.size() > 10)
        return false;

    bool hasNumber = false;
    bool hasSymbol = false;

    for (unsigned char ch : password)
    {
        if (ch >= '0' && ch <= '9') hasNumber = true;
        else if (!((ch >= 'a' && ch <= 'z') ||
                   (ch >= 'A' && ch <= 'Z')))
            hasSymbol = true;
    }
    return hasNumber && hasSymbol;
}

std::string hashPassword(const std::string& password)
{
    unsigned char salt[SALT_SIZE];
    unsigned char derived[HASH_SIZE];

    if (RAND_bytes(salt, sizeof(salt)) != 1)
        return "";

    if (PKCS5_PBKDF2_HMAC(password.c_str(),
                          static_cast<int>(password.size()),
                          salt, sizeof(salt), ITERATIONS,
                          EVP_sha256(), sizeof(derived), derived) != 1)
        return "";

    return "pbkdf2_sha256$" + std::to_string(ITERATIONS) + "$" +
           toHex(salt, sizeof(salt)) + "$" +
           toHex(derived, sizeof(derived));
}

bool verifyPassword(const std::string& password, const std::string& storedValue)
{
    const std::string prefix = "pbkdf2_sha256$";
    if (storedValue.compare(0, prefix.size(), prefix) != 0)
        return password == storedValue; // Legacy plain-text account; force reset after login.

    size_t p1 = storedValue.find('$');
    size_t p2 = storedValue.find('$', p1 + 1);
    size_t p3 = storedValue.find('$', p2 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos ||
        p3 == std::string::npos) return false;

    int iterations = 0;
    try { iterations = std::stoi(storedValue.substr(p1 + 1, p2 - p1 - 1)); }
    catch (...) { return false; }
    if (iterations < 100000 || iterations > 1000000) return false;

    std::vector<unsigned char> salt, expected;
    if (!fromHex(storedValue.substr(p2 + 1, p3 - p2 - 1), salt) ||
        !fromHex(storedValue.substr(p3 + 1), expected) ||
        salt.size() != SALT_SIZE || expected.size() != HASH_SIZE)
        return false;

    unsigned char actual[HASH_SIZE];
    if (PKCS5_PBKDF2_HMAC(password.c_str(),
                          static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          sizeof(actual), actual) != 1)
        return false;

    return CRYPTO_memcmp(actual, expected.data(), HASH_SIZE) == 0;
}

bool login(const std::string& username, const std::string& password,
           int& userId, std::string& role, bool& mustChangePassword)
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, password, role FROM users WHERE username=?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool success = false;
    mustChangePassword = false;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        userId = sqlite3_column_int(stmt, 0);
        const unsigned char* saved = sqlite3_column_text(stmt, 1);
        const unsigned char* savedRole = sqlite3_column_text(stmt, 2);
        std::string stored = saved ? reinterpret_cast<const char*>(saved) : "";
        role = savedRole ? reinterpret_cast<const char*>(savedRole) : "";

        if (verifyPassword(password, stored))
        {
            success = true;
            if (stored.compare(0, 14, "pbkdf2_sha256$") != 0 ||
                !isValidPassword(password))
                mustChangePassword = true;
        }
    }

    sqlite3_finalize(stmt);
    return success;
}

bool changePassword(int userId, const std::string& newPassword)
{
    if (!isValidPassword(newPassword)) return false;

    std::string encoded = hashPassword(newPassword);
    if (encoded.empty()) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "UPDATE users SET password=? WHERE id=?", -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, encoded.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) == 1;
    sqlite3_finalize(stmt);
    return ok;
}
