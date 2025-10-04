#pragma once

#ifndef HASH_GENERATOR_H
#define HASH_GENERATOR_H

#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

using namespace std;

class SecurePasswordHasher {
 public:
  static string generateSalt(size_t length = 16) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);

    string salt;
    for (size_t i = 0; i < length; ++i) {
      salt += static_cast<char>(dis(gen));
    }
    return bytesToHex((unsigned char*)salt.c_str(), length);
  }

  static string hashPassword(const string& password) {
    string salt = generateSalt(16);

    // Создаем salted password и хэшируем
    string saltedPassword = salt + password;
    size_t hashValue = hash<string>{}(saltedPassword);

    // Формат: salt|hash (используем другой разделитель)
    return salt + "|" +
           bytesToHex((unsigned char*)&hashValue, sizeof(hashValue));
  }

  static bool verifyPassword(const string& password, const string& storedHash) {
    // Ищем наш разделитель |
    size_t delimiter = storedHash.find('|');
    if (delimiter == string::npos) return false;

    string salt = storedHash.substr(0, delimiter);
    string originalHash = storedHash.substr(delimiter + 1);

    string saltedPassword = salt + password;
    size_t hashValue = hash<string>{}(saltedPassword);
    string newHash = bytesToHex((unsigned char*)&hashValue, sizeof(hashValue));

    return originalHash == newHash;
  }

 private:
  static string bytesToHex(unsigned char* data, size_t length) {
    stringstream ss;
    ss << hex << setfill('0');
    for (size_t i = 0; i < length; ++i) {
      ss << setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
  }
};

#endif