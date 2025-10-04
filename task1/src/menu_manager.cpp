#include "menu_manager.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "database.h"
#include "input_validator.h"

using namespace std;

// Реализация вспомогательных функций
bool hasPermission(Role userRole, Role requiredRole) {
  return static_cast<int>(userRole) >= static_cast<int>(requiredRole);
}

string getRoleName(Role role) {
  switch (role) {
    case Role::ADMIN:
      return "Администратор";
    case Role::USER:
      return "Пользователь";
    case Role::GUEST:
      return "Гость";
    default:
      return "Неизвестно";
  }
}

MenuManager::MenuManager(UserDatabase& db, SecurityLogger& logger,
                         PasswordPolicy& policy, AuthManager& auth,
                         CalculatorEngine& calc)
    : userDB(db),
      securityLogger(logger),
      passwordPolicy(policy),
      authManager(auth),
      calculatorEngine(calc) {}

bool MenuManager::validatePermission(const UserSession& session,
                                     char operation) {
  if ((operation == '!' && !hasPermission(session.role, Role::USER)) ||
      ((operation == '^' || operation == 's' || operation == 'l') &&
       !hasPermission(session.role, Role::ADMIN))) {
    cout << "ОШИБКА: Недостаточно прав для выполнения этой операции!" << endl;
    return false;
  }
  return true;
}

void MenuManager::displayCalculatorMenu(const UserSession& session) {
  cout << "\n" << string(50, '=') << endl;
  cout << "    НАУЧНЫЙ КАЛЬКУЛЯТОР" << endl;
  cout << "Роль: " << getRoleName(session.role) << endl;
  cout << string(50, '=') << endl;

  cout << "Поддерживаемые операции:" << endl;
  cout << "  +  - Сложение" << endl;
  cout << "  -  - Вычитание" << endl;
  cout << "  *  - Умножение" << endl;
  cout << "  /  - Деление" << endl;

  if (hasPermission(session.role, Role::USER)) {
    cout << "  !  - Факториал (только целые числа)" << endl;
  }

  if (hasPermission(session.role, Role::ADMIN)) {
    cout << "  ^  - Возведение в степень" << endl;
    cout << "  s  - Квадратный корень" << endl;
    cout << "  l  - Логарифм (натуральный)" << endl;
  }

  cout << "  q  - Выход" << endl;
  cout << string(50, '=') << endl;
}

void MenuManager::handleBasicOperations(char op, const UserSession& session) {
  if (!validatePermission(session, op)) return;

  double num1 = InputValidator::getValidatedDouble("Введите первое число: ");
  double num2 = InputValidator::getValidatedDouble("Введите второе число: ");

  cout << "\nВычисление: " << num1 << " " << op << " " << num2 << " = ";

  auto result = calculatorEngine.calculate(op, num1, num2);
  if (result.success) {
    cout << result.value << endl;
  } else {
    cout << "ОШИБКА: " << result.errorMessage << endl;
  }
}

void MenuManager::handleAdvancedOperations(char op,
                                           const UserSession& session) {
  if (!validatePermission(session, op)) return;

  if (op == '!') {
    int num = InputValidator::getValidatedInt("Введите целое число: ");
    cout << "\nВычисление: " << num << "! = ";

    auto result =
        calculatorEngine.calculateAdvanced(op, static_cast<double>(num));
    if (result.success) {
      cout << result.value << endl;
    } else {
      cout << "ОШИБКА: " << result.errorMessage << endl;
    }
  } else {
    double num = InputValidator::getValidatedDouble("Введите число: ");
    string operationName = (op == 's')   ? "√" + to_string(num)
                           : (op == 'l') ? "ln(" + to_string(num) + ")"
                                         : "";

    cout << "\nВычисление: " << operationName << " = ";

    auto result = calculatorEngine.calculateAdvanced(op, num);
    if (result.success) {
      cout << result.value << endl;
    } else {
      cout << "ОШИБКА: " << result.errorMessage << endl;
    }
  }
}

void MenuManager::showCalculator(const UserSession& session) {
  char op;

  while (true) {
    displayCalculatorMenu(session);
    cout << "\nВведите операцию: ";
    cin >> op;

    if (op == 'q' || op == 'Q') {
      cout << "Спасибо за использование калькулятора!" << endl;
      break;
    }

    try {
      switch (op) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
          handleBasicOperations(op, session);
          break;
        case '!':
        case 's':
        case 'l':
          handleAdvancedOperations(op, session);
          break;
        default:
          throw runtime_error("Неподдерживаемая операция!");
      }
    } catch (const exception& e) {
      cout << "ОШИБКА: " << e.what() << endl;
    }
  }
}

void MenuManager::showAdminPanel(UserSession& session) {
  while (true) {
    cout << "\n=== ПАНЕЛЬ АДМИНИСТРАТОРА ===" << endl;
    cout << "1. Добавить пользователя" << endl;
    cout << "2. Показать всех пользователей" << endl;
    cout << "3. Изменить роль пользователя" << endl;
    cout << "4. Блокировка/разблокировка пользователя" << endl;
    cout << "5. Удалить пользователя" << endl;
    cout << "6. Показать статистику IP блокировок" << endl;
    cout << "7. Сохранить базу данных" << endl;
    cout << "8. Показать логи безопасности" << endl;
    cout << "9. Вернуться в калькулятор" << endl;
    cout << "0. Выход" << endl;

    int choice = InputValidator::getMenuChoice(0, 9);
    InputValidator::clearInputBuffer();

    switch (choice) {
      case 1: {
        string login;
        cout << "Новый логин: ";
        cin >> login;

        string password;
        bool passwordValid = false;
        do {
          cout << "Новый пароль: ";
          cin >> password;

          auto validation = passwordPolicy.validatePassword(password);
          if (validation.isValid) {
            passwordValid = true;
          } else {
            cout << "Ошибка пароля: " << validation.message << endl;
          }
        } while (!passwordValid);

        int roleChoice = InputValidator::getValidatedInt(
            "Выберите роль (0 - Гость, 1 - Пользователь, 2 - Админ): ");

        Role newRole = static_cast<Role>(roleChoice);
        userDB.addUser(login, password, newRole);
        cout << "Пользователь " << login << " добавлен!" << endl;
        securityLogger.logAdminAction(session.username, "add_user", login);
        break;
      }
      case 2: {
        cout << "\nЗарегистрированные пользователи:" << endl;
        cout << string(55, '-') << endl;
        cout << left << setw(15) << "Логин" << setw(20) << "Роль" << setw(15)
             << "Статус" << endl;
        cout << string(55, '-') << endl;

        for (const auto& user : userDB.getAllUsers()) {
          cout << left << setw(15) << user.first << setw(20)
               << getRoleName(user.second.role) << setw(15)
               << (user.second.isActive ? "Активен" : "Заблокирован") << endl;
        }
        break;
      }
      case 3: {
        string login;
        int newRoleChoice;
        cout << "Введите логин пользователя: ";
        cin >> login;

        UserInfo* userInfo = userDB.getUser(login);
        if (userInfo) {
          cout << "Текущая роль: " << getRoleName(userInfo->role) << endl;
          cout << "Новая роль (0 - Гость, 1 - Пользователь, 2 - Админ): ";
          cin >> newRoleChoice;

          Role newRole = Role::GUEST;
          if (newRoleChoice == 1)
            newRole = Role::USER;
          else if (newRoleChoice == 2)
            newRole = Role::ADMIN;

          if (userDB.updateUserRole(login, newRole)) {
            cout << "Роль пользователя " << login << " изменена на "
                 << getRoleName(newRole) << endl;
            securityLogger.logAdminAction(session.username, "change_role",
                                          login);
          } else {
            cout << "Ошибка при изменении роли!" << endl;
          }
        } else {
          cout << "Пользователь не найден!" << endl;
        }
        break;
      }
      case 4: {
        string login;
        cout << "Введите логин пользователя: ";
        cin >> login;

        UserInfo* userInfo = userDB.getUser(login);
        if (userInfo) {
          if (userDB.toggleUserActive(login)) {
            bool newStatus = userDB.getUser(login)->isActive;
            cout << "Пользователь " << login << " теперь "
                 << (newStatus ? "активен" : "заблокирован") << endl;
            securityLogger.logAdminAction(
                session.username, newStatus ? "unblock_user" : "block_user",
                login);
          } else {
            cout << "Ошибка при изменении статуса!" << endl;
          }
        } else {
          cout << "Пользователь не найден!" << endl;
        }
        break;
      }
      case 5: {
        string login;
        cout << "Введите логин пользователя для удаления: ";
        cin >> login;

        if (login == session.username) {
          cout << "Нельзя удалить свою учетную запись!" << endl;
        } else if (userDB.deleteUser(login)) {
          cout << "Пользователь " << login << " удален!" << endl;
          securityLogger.logAdminAction(session.username, "delete_user", login);
        } else {
          cout << "Пользователь не найден!" << endl;
        }
        break;
      }
      case 6: {
        cout << "\n=== СТАТИСТИКА БЕЗОПАСНОСТИ ===" << endl;
        cout << "Текущий IP сессии: " << session.ipAddress << endl;
        IPLockInfo ipInfo = userDB.getIPLockInfo(session.ipAddress);
        cout << "Неудачных попыток с текущего IP: " << ipInfo.attempts << endl;
        if (userDB.isIPLocked(session.ipAddress)) {
          time_t remaining =
              userDB.getIPUnlockTime(session.ipAddress) - time(nullptr);
          cout << "Статус: ЗАБЛОКИРОВАН (разблокировка через " << remaining
               << " сек.)" << endl;
        } else {
          cout << "Статус: АКТИВЕН" << endl;
        }
        break;
      }
      case 7: {
        if (userDB.saveUsers()) {
          cout << "База данных успешно сохранена!" << endl;
          securityLogger.logAdminAction(session.username, "save_database", "");
        } else {
          cout << "Ошибка при сохранении базы данных!" << endl;
        }
        break;
      }
      case 8: {
        cout << "\n=== ЛОГИ БЕЗОПАСНОСТИ ===" << endl;
        cout << "Логи записываются в файл: security.log" << endl;
        cout << "Для просмотра используйте команду: tail -f security.log"
             << endl;
        break;
      }
      case 9: {
        showCalculator(session);
        break;
      }
      case 0: {
        cout << "Выход из системы..." << endl;
        if (userDB.saveUsers()) {
          cout << "Изменения сохранены." << endl;
        } else {
          cout << "Предупреждение: Не удалось сохранить изменения!" << endl;
        }
        exit(0);
      }
      default: {
        cout << "Неверный выбор!" << endl;
        break;
      }
    }
  }
}

void MenuManager::showChangePassword(const UserSession& session) {
  string currentPassword, newPassword, confirmPassword;

  cout << "\n=== СМЕНА ПАРОЛЯ ===" << endl;
  cout << "Текущий пароль: ";
  cin >> currentPassword;

  UserInfo* userInfo = userDB.getUser(session.username);
  if (userInfo && SecurePasswordHasher::verifyPassword(
                      currentPassword, userInfo->passwordHash)) {
    bool passwordValid = false;
    do {
      cout << "Новый пароль: ";
      cin >> newPassword;

      auto validation = passwordPolicy.validatePassword(newPassword);
      if (validation.isValid) {
        passwordValid = true;
      } else {
        cout << "Ошибка пароля: " << validation.message << endl;
      }
    } while (!passwordValid);

    cout << "Подтвердите новый пароль: ";
    cin >> confirmPassword;

    if (newPassword == confirmPassword) {
      userInfo->passwordHash = SecurePasswordHasher::hashPassword(newPassword);
      cout << "Пароль успешно изменен!" << endl;
      securityLogger.logPasswordChange(session.username, true);
    } else {
      cout << "Пароли не совпадают!" << endl;
      securityLogger.logPasswordChange(session.username, false);
    }
  } else {
    cout << "Неверный текущий пароль!" << endl;
    securityLogger.logPasswordChange(session.username, false);
  }
}

void MenuManager::showUserMenu(UserSession& session) {
  while (true) {
    cout << "\n=== ГЛАВНОЕ МЕНЮ ===" << endl;
    cout << "1. Калькулятор" << endl;
    cout << "2. Сменить пароль" << endl;
    if (hasPermission(session.role, Role::ADMIN)) {
      cout << "3. Панель администратора" << endl;
      cout << "4. Выход" << endl;
    } else {
      cout << "3. Выход" << endl;
    }

    int choice = InputValidator::getMenuChoice(
        1, hasPermission(session.role, Role::ADMIN) ? 4 : 3);

    switch (choice) {
      case 1:
        showCalculator(session);
        break;
      case 2:
        showChangePassword(session);
        break;
      case 3:
        if (hasPermission(session.role, Role::ADMIN)) {
          showAdminPanel(session);
        } else {
          cout << "Выход из системы..." << endl;
          return;
        }
        break;
      case 4:
        cout << "Выход из системы..." << endl;
        return;
    }
  }
}