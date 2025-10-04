#pragma once

#ifndef PASSWORD_POLICY_H
#define PASSWORD_POLICY_H

#include <regex>
#include <string>
#include <vector>

using namespace std;

class PasswordPolicy {
 private:
  size_t minLength = 8;
  bool requireUpper = true;
  bool requireLower = true;
  bool requireDigits = true;
  bool requireSpecial = true;
  vector<string> commonPasswords = {"password", "admin", "111", "123"};

 public:
  struct ValidationResult {
    bool isValid;
    string message;
  };

  ValidationResult validatePassword(const string& password) {
    if (password.length() < minLength) {
      return {false, "Пароль должен содержать не менее " +
                         to_string(minLength) + " символов"};
    }

    if (requireUpper && !regex_search(password, regex("[A-ZА-Я]"))) {
      return {false, "Пароль должен содержать заглавные буквы"};
    }

    if (requireLower && !regex_search(password, regex("[a-zа-я]"))) {
      return {false, "Пароль должен содержать строчные буквы"};
    }

    if (requireDigits && !regex_search(password, regex("[0-9]"))) {
      return {false, "Пароль должен содержать цифры"};
    }

    if (requireSpecial &&
        !regex_search(password,
                      regex("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]"))) {
      return {false, "Пароль должен содержать специальные символы"};
    }

    // Проверка на распространенные пароли
    string lowerPassword = password;
    transform(lowerPassword.begin(), lowerPassword.end(), lowerPassword.begin(),
              ::tolower);

    for (const auto& common : commonPasswords) {
      if (lowerPassword.find(common) != string::npos) {
        return {false,
                "Отказано по причинам безопасности. Пароль содержит "
                "распространённую последовательность символов."};
      }
    }

    return {true, "Пароль надежен"};
  }

  void setMinLength(int length) { minLength = length; }
  void setRequireUpper(bool require) { requireUpper = require; }
  void setRequireLower(bool require) { requireLower = require; }
  void setRequireDigits(bool require) { requireDigits = require; }
  void setRequireSpecial(bool require) { requireSpecial = require; }
};

#endif