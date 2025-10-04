#pragma once

#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <ctime>
#include <map>
#include <string>

#include "database.h"
#include "security_logger.h"

using namespace std;

struct LockInfo {
  int attempts;
  time_t unlockTime;
};

struct UserSession {
  string username;
  Role role;
  string ipAddress;
};

class AuthManager {
 private:
  UserDatabase& userDB;
  SecurityLogger& securityLogger;
  map<string, LockInfo> loginAttempts;

  const int MAX_ACCOUNT_ATTEMPTS = 3;
  const int ACCOUNT_LOCK_TIME = 30;
  const int MAX_IP_ATTEMPTS = 10;
  const int IP_LOCK_TIME = 60;

  bool isAccountLocked(const string& login);
  void showIPLockInfo(const string& ip);
  string getClientIP();

 public:
  AuthManager(UserDatabase& db, SecurityLogger& logger);
  UserSession authenticate();
  void resetAttempts(const string& login, const string& ip);
};

string getRoleName(Role role);

#endif