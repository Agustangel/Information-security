#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "hash_generator.h"

using namespace std;

// Роли пользователей
enum class Role {
  GUEST,  // Только базовые операции
  USER,   // Базовые операции + дополнительные
  ADMIN  // Все операции + управление пользователями
};

// Структура для хранения информации о пользователе
struct UserInfo {
  string passwordHash;
  Role role;
  bool isActive;
};

// Класс для работы с базой данных пользователей
class UserDatabase {
 private:
  string dbFilename;
  map<string, UserInfo> users;

  string simpleEncrypt(const string& data, const string& key) {
    string encrypted = data;
    for (size_t i = 0; i < data.length(); ++i) {
      encrypted[i] = data[i] ^ key[i % key.length()];
    }
    return encrypted;
  }

  string simpleDecrypt(const string& data, const string& key) {
    return simpleEncrypt(data, key);  // XOR обратим
  }

  void setFilePermissions() {
    chmod(dbFilename.c_str(),
          S_IRUSR | S_IWUSR);  // Только владелец может читать/писать
  }

 public:
  UserDatabase(const string& filename = "users.dat") : dbFilename(filename) {}

  bool loadUsers(const string& encryptionKey = "default_key_123") {
    ifstream file(dbFilename, ios::binary);
    if (!file.is_open()) {
      cout << "База пользователей не найдена. Создана новая." << endl;
      createDefaultUsers();
      return saveUsers(encryptionKey);
    }

    string encryptedData((istreambuf_iterator<char>(file)),
                         istreambuf_iterator<char>());
    file.close();

    if (encryptedData.empty()) {
      cout << "База пользователей пуста." << endl;
      createDefaultUsers();
      return saveUsers(encryptionKey);
    }
    string data = encryptedData;

    // Парсинг данных
    stringstream ss(data);
    string line;
    users.clear();
    int loadedCount = 0;

    while (getline(ss, line)) {
      if (line.empty()) continue;

      // Разбираем строку вручную, учитывая экранирование
      vector<string> parts;
      string part;
      bool escaped = false;

      for (char c : line) {
        if (escaped) {
          part += c;
          escaped = false;
        } else if (c == '\\') {
          escaped = true;
        } else if (c == ':') {
          parts.push_back(part);
          part.clear();
        } else {
          part += c;
        }
      }
      parts.push_back(part);

      if (parts.size() == 4) {
        try {
          string login = parts[0];
          int role = stoi(parts[1]);
          int active = stoi(parts[2]);
          string passwordHash = parts[3];
          if (role < 0 || role > 2) {
            cout << "Некорректная роль для пользователя " << login << ": "
                 << role << endl;
            continue;
          }
          users[login] = {passwordHash, static_cast<Role>(role),
                          static_cast<bool>(active)};
          loadedCount++;
        } catch (const exception& e) {
          cout << "Ошибка при загрузке пользователя: " << e.what()
               << " (данные: " << line << ")" << endl;
        }
      } else {
        cout << "Некорректный формат строки (ожидалось 4 части, получили "
             << parts.size() << "): " << line << endl;
      }
    }
    cout << "Загружено пользователей: " << loadedCount << endl;
    if (users.empty()) {
      cout << "Создана новая база пользователей по умолчанию." << endl;
      createDefaultUsers();
      return saveUsers(encryptionKey);
    }

    return true;
  }

  bool saveUsers(const string& encryptionKey = "default_key_123") {
    ofstream file(dbFilename, ios::binary);
    if (!file.is_open()) {
      cerr << "Ошибка: Не удалось открыть файл для записи: " << dbFilename
           << endl;
      return false;
    }
    // Сериализация данных
    stringstream data;
    for (const auto& [login, userInfo] : users) {
      string escapedLogin = login;
      size_t pos = 0;
      while ((pos = escapedLogin.find(':', pos)) != string::npos) {
        escapedLogin.replace(pos, 1, "\\:");
        pos += 2;
      }
      data << escapedLogin << ":" << static_cast<int>(userInfo.role) << ":"
           << (userInfo.isActive ? "1" : "0") << ":" << userInfo.passwordHash
           << "\n";
    }

    string dataStr = data.str();
    string output = dataStr;
    file << output;
    if (!file.good()) {
      cerr << "Ошибка при записи в файл!" << endl;
      return false;
    }
    file.close();

    setFilePermissions();
    cout << "База данных успешно сохранена (" << users.size()
         << " пользователей)" << endl;

    return true;
  }

  void createDefaultUsers() {
    users = {
        {"admin",
         {SecurePasswordHasher::hashPassword("admin123"), Role::ADMIN, true}},
        {"user1",
         {SecurePasswordHasher::hashPassword("user123"), Role::USER, true}},
        {"guest",
         {SecurePasswordHasher::hashPassword("guest123"), Role::GUEST, true}}};
  }

  // Методы доступа к пользователям
  bool userExists(const string& login) const {
    return users.find(login) != users.end();
  }

  UserInfo* getUser(const string& login) {
    auto it = users.find(login);
    return it != users.end() ? &it->second : nullptr;
  }

  const map<string, UserInfo>& getAllUsers() const { return users; }

  void addUser(const string& login, const string& password, Role role) {
    users[login] = {SecurePasswordHasher::hashPassword(password), role, true};
  }

  bool updateUserRole(const string& login, Role newRole) {
    auto it = users.find(login);
    if (it != users.end()) {
      it->second.role = newRole;
      return true;
    }
    return false;
  }

  bool toggleUserActive(const string& login) {
    auto it = users.find(login);
    if (it != users.end()) {
      it->second.isActive = !it->second.isActive;
      return true;
    }
    return false;
  }

  bool deleteUser(const string& login) { return users.erase(login) > 0; }
};

#endif