#include <locale.h>

#include <iostream>

#include "auth_manager.h"
#include "calculator_engine.h"
#include "database.h"
#include "hash_generator.h"
#include "input_validator.h"
#include "menu_manager.h"
#include "password_policy.h"
#include "security_logger.h"

using namespace std;

int main() {
  setlocale(LC_ALL, "Russian");

  cout << "=========================================" << endl;
  cout << "    БЕЗОПАСНЫЙ КАЛЬКУЛЯТОР" << endl;
  cout << "=========================================" << endl;

  // Инициализация компонентов
  UserDatabase userDB;
  SecurityLogger securityLogger;
  PasswordPolicy passwordPolicy;
  AuthManager authManager(userDB, securityLogger);
  CalculatorEngine calculatorEngine;
  MenuManager menuManager(userDB, securityLogger, passwordPolicy, authManager,
                          calculatorEngine);

  // Логируем запуск приложения
  securityLogger.logSecurityEvent("Application started",
                                  "Modular Secure Calculator v2.0");

  // Загрузка базы данных
  if (!userDB.loadUsers()) {
    cerr << "Критическая ошибка: Не удалось загрузить базу пользователей!"
         << endl;
    securityLogger.logSecurityEvent("Critical error",
                                    "Failed to load user database");
    return 1;
  }

  // Аутентификация пользователя
  UserSession session = authManager.authenticate();

  if (!session.username.empty()) {
    menuManager.showUserMenu(session);

    // Сохраняем базу данных при выходе
    if (userDB.saveUsers()) {
      cout << "Изменения сохранены." << endl;
      securityLogger.logSecurityEvent("Application shutdown",
                                      "Normal termination");
    } else {
      cout << "Предупреждение: Не удалось сохранить изменения!" << endl;
      securityLogger.logSecurityEvent("Application shutdown", "Save error");
    }
  } else {
    cout << "Программа завершена по соображениям безопасности." << endl;
    securityLogger.logSecurityEvent("Application shutdown",
                                    "Security termination");
    return 1;
  }

  cout << "До свидания!" << endl;
  return 0;
}
