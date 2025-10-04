#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "database.h"
#include "hash_generator.h"
#include "password_policy.h"
#include "security_logger.h"

using namespace std;

// Глобальная база данных пользователей
UserDatabase userDB;

// Глобальные объекты для безопасности
SecurityLogger securityLogger;
PasswordPolicy passwordPolicy;

map<string, LockInfo> loginAttempts;

string getClientIP() {
  return "127.0.0.1";  // Локальный IP для Linux/Mac
}

// Функция для проверки роли
bool hasPermission(Role userRole, Role requiredRole) {
  return static_cast<int>(userRole) >= static_cast<int>(requiredRole);
}

// Функция для получения названия роли
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

// Функция для проверки и обновления блокировки
bool isAccountLocked(const string& login) {
  if (loginAttempts.find(login) != loginAttempts.end()) {
    LockInfo& info = loginAttempts[login];

    if (info.attempts >= 3) {
      time_t now = time(nullptr);

      if (now < info.unlockTime) {
        int remaining = info.unlockTime - now;
        cout << "Аккаунт заблокирован. Попробуйте снова через " << remaining
             << " секунд." << endl;
        return true;
      } else {
        // Время блокировки истекло, сбрасываем счетчик
        info.attempts = 0;
      }
    }
  }
  return false;
}

// Функция для отображения информации о блокировке IP
void showIPLockInfo(const string& ip) {
  IPLockInfo ipInfo = userDB.getIPLockInfo(ip);
  time_t now = time(nullptr);

  if (userDB.isIPLocked(ip)) {
    time_t unlockTime = userDB.getIPUnlockTime(ip);
    int remaining = unlockTime - now;
    cout << "\n ВНИМАНИЕ: Ваш IP заблокирован за слишком много неудачных "
            "попыток входа!"
         << endl;
    cout << "Разблокировка через: " << remaining << " секунд" << endl;
    cout << "Всего попыток: " << ipInfo.attempts << endl;
  } else if (ipInfo.attempts > 0) {
    cout << "\n Статистика безопасности:" << endl;
    cout << "Неудачных попыток с вашего IP: " << ipInfo.attempts << "/10"
         << endl;
    cout << "После 10 неудачных попыток IP будет заблокирован на 1 минуту"
         << endl;
  }
}

// Функция аутентификации с временной блокировкой
UserSession authenticate() {
  const int max_attempts = 3;
  const int lock_time = 30;
  string login, password;

  cout << "=== СИСТЕМА АУТЕНТИФИКАЦИИ ===" << endl;
  cout << "Максимальное количество попыток на аккаунт: " << max_attempts
       << endl;
  cout << "Время блокировки аккаунта: " << lock_time << " секунд" << endl;
  cout << "Максимальное количество попыток с IP: 10" << endl;
  cout << "Время блокировки IP: 1 минута" << endl;

  string clientIP = getClientIP();
  cout << "Ваш IP: " << clientIP << endl;

  while (true) {
    // Проверка блокировки IP
    if (userDB.isIPLocked(clientIP)) {
      showIPLockInfo(clientIP);
      securityLogger.logSecurityEvent("IP blocked", "ip=" + clientIP);
      // Ждем разблокировки
      while (userDB.isIPLocked(clientIP)) {
        time_t unlockTime = userDB.getIPUnlockTime(clientIP);
        time_t now = time(nullptr);
        int remaining = unlockTime - now;

        if (remaining <= 0) break;

        cout << "Ожидание разблокировки... " << remaining << " секунд" << endl;
        sleep(10);  // 10 секунд для Linux/Mac
      }
      cout << "IP разблокирован! Продолжаем..." << endl;
      securityLogger.logSecurityEvent("IP unblocked", "ip=" + clientIP);
    }

    cout << "\n Логин: ";
    cin >> login;

    if (isAccountLocked(login)) {
      securityLogger.logLoginFailure(login, clientIP, "Account locked");
      userDB.registerFailedAttempt(clientIP);
      showIPLockInfo(clientIP);
      continue;
    }

    // Проверка существования пользователя и активности учетной записи
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
      cout << "\n Доступ разрешен! Добро пожаловать, " << login << "!" << endl;
      cout << "Ваша роль: " << getRoleName(userInfo->role) << endl;

      // Логируем успешный вход
      securityLogger.logLoginSuccess(login, clientIP);

      // Сбрасываем счетчики при успешном входе
      if (loginAttempts.find(login) != loginAttempts.end()) {
        loginAttempts[login].attempts = 0;
      }
      userDB.resetIPAttempts(clientIP);

      return {login, userInfo->role, clientIP};
    } else {
      // Регистрируем неудачную попытку
      if (loginAttempts.find(login) == loginAttempts.end()) {
        loginAttempts[login] = {1, 0};
      } else {
        loginAttempts[login].attempts++;
      }

      userDB.registerFailedAttempt(clientIP);

      LockInfo& info = loginAttempts[login];
      int remaining = max_attempts - info.attempts;

      // Логируем неудачную попытку
      string failureReason = userInfo ? "Wrong password" : "User not found";
      securityLogger.logLoginFailure(login, clientIP, failureReason);

      if (info.attempts >= max_attempts) {
        info.unlockTime = time(nullptr) + lock_time;
        cout << "\n Превышено максимальное количество попыток для аккаунта!"
             << endl;
        cout << "Аккаунт заблокирован на " << lock_time << " секунд." << endl;
        securityLogger.logSecurityEvent("Account locked",
                                        "user=" + login + " ip=" + clientIP);
      } else if (remaining > 0) {
        cout << "Неверные данные. Осталось попыток для аккаунта: " << remaining
             << endl;
      }

      // Показываем информацию о блокировке IP
      showIPLockInfo(clientIP);
    }
  }
}

// Функция для вычисления факториала (только для USER и ADMIN)
long long factorial(int n) {
  long long result = 1;
  for (int i = 2; i <= n; ++i) {
    result *= i;
  }
  return result;
}

// Функция для возведения в степень (только для ADMIN)
double power(double base, double exponent) { return pow(base, exponent); }

void calculator(const UserSession& session) {
  char op;
  double num1, num2, result;
  int intNum;

  cout << "\n" << string(50, '=') << endl;
  cout << "    НАУЧНЫЙ КАЛЬКУЛЯТОР" << endl;
  cout << "Роль: " << getRoleName(session.role) << endl;
  cout << string(50, '=') << endl;

  cout << "Поддерживаемые операции:" << endl;
  cout << "  +  - Сложение" << endl;
  cout << "  -  - Вычитание" << endl;
  cout << "  *  - Умножение" << endl;
  cout << "  /  - Деление" << endl;

  // Дополнительные операции для USER и ADMIN
  if (hasPermission(session.role, Role::USER)) {
    cout << "  !  - Факториал (только целые числа)" << endl;
  }

  // Расширенные операции только для ADMIN
  if (hasPermission(session.role, Role::ADMIN)) {
    cout << "  ^  - Возведение в степень" << endl;
    cout << "  s  - Квадратный корень" << endl;
    cout << "  l  - Логарифм (натуральный)" << endl;
  }

  cout << "  q  - Выход" << endl;
  cout << string(50, '=') << endl;

  while (true) {
    cout << "\nВведите операцию: ";
    cin >> op;

    if (op == 'q' || op == 'Q') {
      cout << "Спасибо за использование калькулятора!" << endl;
      break;
    }

    // Проверка прав доступа для операций
    if ((op == '!' && !hasPermission(session.role, Role::USER)) ||
        ((op == '^' || op == 's' || op == 'l') &&
         !hasPermission(session.role, Role::ADMIN))) {
      cout << "ОШИБКА: Недостаточно прав для выполнения этой операции!" << endl;
      continue;
    }

    try {
      switch (op) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '^':
          cout << "Введите первое число: ";
          while (!(cin >> num1)) {
            cout << "Ошибка! Введите корректное число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "Введите второе число: ";
          while (!(cin >> num2)) {
            cout << "Ошибка! Введите корректное число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "\n Вычисление: " << num1 << " " << op << " " << num2
               << " = ";

          switch (op) {
            case '+':
              result = num1 + num2;
              break;
            case '-':
              result = num1 - num2;
              break;
            case '*':
              result = num1 * num2;
              break;
            case '/':
              if (num2 != 0) {
                result = num1 / num2;
              } else {
                throw runtime_error("Деление на ноль!");
              }
              break;
            case '^':
              result = power(num1, num2);
              break;
          }
          cout << result << endl;
          break;

        case '!':
          cout << "Введите целое число: ";
          while (!(cin >> intNum)) {
            cout << "Ошибка! Введите целое число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "\n Вычисление: " << intNum << "! = ";
          if (intNum < 0) {
            throw runtime_error("Факториал отрицательного числа не определен!");
          }
          if (intNum > 20) {
            throw runtime_error("Слишком большое число для факториала!");
          }
          result = static_cast<double>(factorial(intNum));
          cout << result << endl;
          break;

        case 's':
          cout << "Введите число: ";
          while (!(cin >> num1)) {
            cout << "Ошибка! Введите корректное число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "\n Вычисление: √" << num1 << " = ";
          if (num1 < 0) {
            throw runtime_error("Квадратный корень из отрицательного числа!");
          }
          result = sqrt(num1);
          cout << result << endl;
          break;

        case 'l':
          cout << "Введите число: ";
          while (!(cin >> num1)) {
            cout << "Ошибка! Введите корректное число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "\n Вычисление: ln(" << num1 << ") = ";
          if (num1 <= 0) {
            throw runtime_error(
                "Логарифм определен только для положительных чисел!");
          }
          result = log(num1);
          cout << result << endl;
          break;

        default:
          throw runtime_error("Неподдерживаемая операция!");
      }
    } catch (const exception& e) {
      cout << "ОШИБКА: " << e.what() << endl;
    }
  }
}

void adminPanel(UserSession& session) {
  int choice;
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
    cout << "Выберите опцию: ";

    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
      string login, password;
      int roleChoice;
      cout << "Новый логин: ";
      cin >> login;

      // Проверка политики паролей
      bool passwordValid = false;
      do {
        cout << "Новый пароль: ";
        cin >> password;

        auto validation = passwordPolicy.validatePassword(password);
        if (validation.isValid) {
          passwordValid = true;
        } else {
          cout << "Ошибка пароля: " << validation.message << endl;
          cout << "Требования: минимум 6 символов, заглавные и строчные буквы, "
                  "цифры, специальные символы"
               << endl;
        }
      } while (!passwordValid);

      cout << "Выберите роль (0 - Гость, 1 - Пользователь, 2 - Админ): ";
      cin >> roleChoice;

      Role newRole = Role::GUEST;
      if (roleChoice == 1)
        newRole = Role::USER;
      else if (roleChoice == 2)
        newRole = Role::ADMIN;

      userDB.addUser(login, password, newRole);
      cout << "Пользователь " << login << " добавлен с ролью "
           << getRoleName(newRole) << "!" << endl;

      securityLogger.logAdminAction(session.username, "add_user", login);

    } else if (choice == 2) {
      cout << "\n Зарегистрированные пользователи:" << endl;
      cout << string(55, '-') << endl;
      cout << left << setw(15) << "Логин" << setw(20) << "Роль" << setw(15)
           << "Статус" << endl;
      cout << string(55, '-') << endl;

      for (const auto& user : userDB.getAllUsers()) {
        cout << left << setw(15) << user.first << setw(20)
             << getRoleName(user.second.role) << setw(15)
             << (user.second.isActive ? "Активен" : "Заблокирован") << endl;
      }

    } else if (choice == 3) {
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
          securityLogger.logAdminAction(session.username, "change_role", login);
        } else {
          cout << "Ошибка при изменении роли!" << endl;
        }
      } else {
        cout << "Пользователь не найден!" << endl;
      }

    } else if (choice == 4) {
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

    } else if (choice == 5) {
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

    } else if (choice == 6) {
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
    } else if (choice == 7) {
      if (userDB.saveUsers()) {
        cout << "База данных успешно сохранена!" << endl;
        securityLogger.logAdminAction(session.username, "save_database", "");
      } else {
        cout << "Ошибка при сохранении базы данных!" << endl;
      }
    } else if (choice == 8) {
      cout << "\n=== ЛОГИ БЕЗОПАСНОСТИ ===" << endl;
      cout << "Логи записываются в файл: security.log" << endl;
      cout << "Для просмотра используйте команду: tail -f security.log" << endl;
    } else if (choice == 9) {
      calculator(session);
    } else if (choice == 0) {
      cout << "Выход из системы..." << endl;
      if (userDB.saveUsers()) {
        cout << "Изменения сохранены." << endl;
      } else {
        cout << "Предупреждение: Не удалось сохранить изменения!" << endl;
      }
      exit(0);
    } else {
      cout << "Неверный выбор!" << endl;
    }
  }
}

void changePassword(UserSession& session) {
  string currentPassword, newPassword, confirmPassword;

  cout << "\n=== СМЕНА ПАРОЛЯ ===" << endl;
  cout << "Текущий пароль: ";
  cin >> currentPassword;

  UserInfo* userInfo = userDB.getUser(session.username);
  if (userInfo && SecurePasswordHasher::verifyPassword(
                      currentPassword, userInfo->passwordHash)) {
    // Проверка нового пароля по политике
    bool passwordValid = false;
    do {
      cout << "Новый пароль: ";
      cin >> newPassword;

      auto validation = passwordPolicy.validatePassword(newPassword);
      if (validation.isValid) {
        passwordValid = true;
      } else {
        cout << "Ошибка пароля: " << validation.message << endl;
        cout << "Требования: минимум 8 символов, заглавные и строчные буквы, "
                "цифры, специальные символы"
             << endl;
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

void userMenu(UserSession& session) {
  int choice;
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
    cout << "Выберите опцию: ";

    cin >> choice;

    if (choice == 1) {
      calculator(session);
    } else if (choice == 2) {
      changePassword(session);
    } else if (choice == 3 && hasPermission(session.role, Role::ADMIN)) {
      adminPanel(session);
    } else if ((choice == 3 && !hasPermission(session.role, Role::ADMIN)) ||
               (choice == 4 && hasPermission(session.role, Role::ADMIN))) {
      cout << "Выход из системы..." << endl;
      break;
    } else {
      cout << "Неверный выбор!" << endl;
    }
  }
}

int main() {
  setlocale(LC_ALL, "Russian");

  cout << "=========================================" << endl;
  cout << "    БЕЗОПАСНЫЙ КАЛЬКУЛЯТОР" << endl;
  cout << "=========================================" << endl;

  // Логируем запуск приложения
  securityLogger.logSecurityEvent("Application started",
                                  "Secure Calculator v1.0");

  // Загрузка базы данных
  if (!userDB.loadUsers()) {
    cerr << "Критическая ошибка: Не удалось загрузить базу пользователей!"
         << endl;
    securityLogger.logSecurityEvent("Critical error",
                                    "Failed to load user database");
    return 1;
  }

  // Аутентификация пользователя
  UserSession session = authenticate();

  if (!session.username.empty()) {
    userMenu(session);

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
