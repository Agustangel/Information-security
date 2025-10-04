#include "auth_manager.h"

#include <unistd.h>

#include <iostream>

using namespace std;

AuthManager::AuthManager(UserDatabase& db, SecurityLogger& logger)
    : userDB(db), securityLogger(logger) {}

string AuthManager::getClientIP() {
  return "127.0.0.1";  // Локальный IP для Linux/Mac
}

bool AuthManager::isAccountLocked(const string& login) {
  if (loginAttempts.find(login) != loginAttempts.end()) {
    LockInfo& info = loginAttempts[login];

    if (info.attempts >= MAX_ACCOUNT_ATTEMPTS) {
      time_t now = time(nullptr);
      if (now < info.unlockTime) {
        int remaining = info.unlockTime - now;
        cout << "Аккаунт заблокирован. Попробуйте снова через " << remaining
             << " секунд." << endl;
        return true;
      } else {
        info.attempts = 0;
      }
    }
  }
  return false;
}

void AuthManager::showIPLockInfo(const string& ip) {
  IPLockInfo ipInfo = userDB.getIPLockInfo(ip);
  time_t now = time(nullptr);

  if (userDB.isIPLocked(ip)) {
    time_t unlockTime = userDB.getIPUnlockTime(ip);
    int remaining = unlockTime - now;
    cout << "\nВНИМАНИЕ: Ваш IP заблокирован за слишком много неудачных "
            "попыток входа!"
         << endl;
    cout << "Разблокировка через: " << remaining << " секунд" << endl;
    cout << "Всего попыток: " << ipInfo.attempts << endl;
  } else if (ipInfo.attempts > 0) {
    cout << "\n Статистика безопасности:" << endl;
    cout << "Неудачных попыток с вашего IP: " << ipInfo.attempts << "/"
         << MAX_IP_ATTEMPTS << endl;
    cout << "После " << MAX_IP_ATTEMPTS
         << " неудачных попыток IP будет заблокирован на " << IP_LOCK_TIME
         << " секунд" << endl;
  }
}

UserSession AuthManager::authenticate() {
  string login, password;

  cout << "=== СИСТЕМА АУТЕНТИФИКАЦИИ ===" << endl;
  cout << "Максимальное количество попыток на аккаунт: " << MAX_ACCOUNT_ATTEMPTS
       << endl;
  cout << "Время блокировки аккаунта: " << ACCOUNT_LOCK_TIME << " секунд"
       << endl;
  cout << "Максимальное количество попыток с IP: " << MAX_IP_ATTEMPTS << endl;
  cout << "Время блокировки IP: " << IP_LOCK_TIME << " секунд" << endl;

  string clientIP = getClientIP();
  cout << "Ваш IP: " << clientIP << endl;

  while (true) {
    if (userDB.isIPLocked(clientIP)) {
      showIPLockInfo(clientIP);
      securityLogger.logSecurityEvent("IP blocked", "ip=" + clientIP);

      while (userDB.isIPLocked(clientIP)) {
        time_t unlockTime = userDB.getIPUnlockTime(clientIP);
        time_t now = time(nullptr);
        int remaining = unlockTime - now;
        if (remaining <= 0) break;

        cout << "Ожидание разблокировки... " << remaining << " секунд" << endl;
        sleep(10);
      }
      cout << "IP разблокирован! Продолжаем..." << endl;
      securityLogger.logSecurityEvent("IP unblocked", "ip=" + clientIP);
    }

    cout << "\nЛогин: ";
    cin >> login;

    if (isAccountLocked(login)) {
      securityLogger.logLoginFailure(login, clientIP, "Account locked");
      userDB.registerFailedAttempt(clientIP);
      showIPLockInfo(clientIP);
      continue;
    }

    UserInfo* userInfo = userDB.getUser(login);
    if (userInfo && !userInfo->isActive) {
      cout << "Учетная запись отключена. Обратитесь к администратору." << endl;
      securityLogger.logLoginFailure(login, clientIP, "Account disabled");
      userDB.registerFailedAttempt(clientIP);
      showIPLockInfo(clientIP);
      continue;
    }

    cout << "Пароль: ";
    cin >> password;

    if (userInfo && SecurePasswordHasher::verifyPassword(
                        password, userInfo->passwordHash)) {
      cout << "\nДоступ разрешен! Добро пожаловать, " << login << "!" << endl;
      cout << "Ваша роль: " << getRoleName(userInfo->role) << endl;

      securityLogger.logLoginSuccess(login, clientIP);
      resetAttempts(login, clientIP);

      return {login, userInfo->role, clientIP};
    } else {
      if (loginAttempts.find(login) == loginAttempts.end()) {
        loginAttempts[login] = {1, 0};
      } else {
        loginAttempts[login].attempts++;
      }

      userDB.registerFailedAttempt(clientIP);

      LockInfo& info = loginAttempts[login];
      int remaining = MAX_ACCOUNT_ATTEMPTS - info.attempts;

      string failureReason = userInfo ? "Wrong password" : "User not found";
      securityLogger.logLoginFailure(login, clientIP, failureReason);

      if (info.attempts >= MAX_ACCOUNT_ATTEMPTS) {
        info.unlockTime = time(nullptr) + ACCOUNT_LOCK_TIME;
        cout << "\nПревышено максимальное количество попыток для аккаунта!"
             << endl;
        cout << "Аккаунт заблокирован на " << ACCOUNT_LOCK_TIME << " секунд."
             << endl;
        securityLogger.logSecurityEvent("Account locked",
                                        "user=" + login + " ip=" + clientIP);
      } else if (remaining > 0) {
        cout << "Неверные данные. Осталось попыток для аккаунта: " << remaining
             << endl;
      }

      showIPLockInfo(clientIP);
    }
  }
}

void AuthManager::resetAttempts(const string& login, const string& ip) {
  if (loginAttempts.find(login) != loginAttempts.end()) {
    loginAttempts[login].attempts = 0;
  }
  userDB.resetIPAttempts(ip);
}