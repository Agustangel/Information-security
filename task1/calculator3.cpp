#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

class SHA256 {
 public:
  static string hash(const string& data) {
    size_t hashValue = std::hash<string>{}(data + "salt123");
    stringstream ss;
    ss << hex << hashValue;
    return ss.str();
  }
};

// База пользователей с ролями
map<string, UserInfo> users = {
    {"admin", {SHA256::hash("password123"), Role::ADMIN, true}},
    {"user1", {SHA256::hash("qwerty"), Role::USER, true}},
    {"test", {SHA256::hash("test"), Role::GUEST, true}}};

// Структура для хранения информации о блокировке
struct LockInfo {
  int attempts;
  time_t unlockTime;
};

map<string, LockInfo> loginAttempts;

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

void addUser(const string& login, const string& password, Role role) {
  users[login] = {SHA256::hash(password), role, true};
  cout << "Пользователь " << login << " добавлен с ролью " << getRoleName(role)
       << "!" << endl;
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

// Структура для сессии пользователя
struct UserSession {
  string username;
  Role role;
};

// Функция аутентификации с временной блокировкой
UserSession authenticate() {
  const int max_attempts = 3;
  const int lock_time = 30;
  string login, password;

  cout << "=== СИСТЕМА АУТЕНТИФИКАЦИИ ===" << endl;
  cout << "Максимальное количество попыток: " << max_attempts << endl;
  cout << "Время блокировки после " << max_attempts
       << " неудачных попыток: " << lock_time << " секунд" << endl;

  while (true) {
    cout << "\nЛогин: ";
    cin >> login;

    if (isAccountLocked(login)) {
      continue;
    }

    // Проверка активности учетной записи
    if (users.find(login) != users.end() && !users[login].isActive) {
      cout << "Учетная запись отключена. Обратитесь к администратору." << endl;
      continue;
    }

    cout << "Пароль: ";
    cin >> password;

    string hashedPassword = SHA256::hash(password);

    if (users.count(login) && users[login].passwordHash == hashedPassword) {
      cout << "\n Доступ разрешен! Добро пожаловать, " << login << "!" << endl;
      cout << "Ваша роль: " << getRoleName(users[login].role) << endl;

      if (loginAttempts.find(login) != loginAttempts.end()) {
        loginAttempts[login].attempts = 0;
      }

      return {login, users[login].role};
    } else {
      if (loginAttempts.find(login) == loginAttempts.end()) {
        loginAttempts[login] = {1, 0};
      } else {
        loginAttempts[login].attempts++;
      }

      LockInfo& info = loginAttempts[login];
      int remaining = max_attempts - info.attempts;

      if (info.attempts >= max_attempts) {
        info.unlockTime = time(nullptr) + lock_time;
        cout << "\nПревышено максимальное количество попыток!" << endl;
        cout << "Аккаунт заблокирован на " << lock_time << " секунд." << endl;
      } else if (remaining > 0) {
        cout << "Неверные данные. Осталось попыток: " << remaining << endl;
      }
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
        ((op == '^' || op == 's') &&
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

          cout << "\nВычисление: " << num1 << " " << op << " " << num2 << " = ";

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

          cout << "\nВычисление: " << intNum << "! = ";
          if (intNum < 0) {
            throw runtime_error("Факториал отрицательного числа не определен!");
          }
          if (intNum > 20) {
            throw runtime_error("Слишком большое число для факториала!");
          }
          result = factorial(intNum);
          cout << result << endl;
          break;

        case 's':
          cout << "Введите число: ";
          while (!(cin >> num1)) {
            cout << "Ошибка! Введите корректное число: ";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
          }

          cout << "\nВычисление: √" << num1 << " = ";
          if (num1 < 0) {
            throw runtime_error("Квадратный корень из отрицательного числа!");
          }
          result = sqrt(num1);
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
    cout << "5. Вернуться в калькулятор" << endl;
    cout << "Выберите опцию: ";

    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
      string login, password;
      int roleChoice;
      cout << "Новый логин: ";
      cin >> login;
      cout << "Новый пароль: ";
      cin >> password;
      cout << "Выберите роль (0 - Гость, 1 - Пользователь, 2 - Админ): ";
      cin >> roleChoice;

      Role newRole = Role::GUEST;
      if (roleChoice == 1)
        newRole = Role::USER;
      else if (roleChoice == 2)
        newRole = Role::ADMIN;

      addUser(login, password, newRole);

    } else if (choice == 2) {
      cout << "\nЗарегистрированные пользователи:" << endl;
      cout << string(42, '-') << endl;
      cout << left << setw(10) << "Логин"
           << " " << right << setw(15) << "Роль"
           << " " << right << setw(20) << "Статус" << endl;
      cout << string(42, '-') << endl;

      for (const auto& user : users) {
        cout << left << setw(10) << user.first << " " << right << setw(15)
             << getRoleName(user.second.role) << " " << right << setw(15)
             << (user.second.isActive ? "Активен" : "Заблокирован") << endl;
      }

    } else if (choice == 3) {
      string login;
      int newRoleChoice;
      cout << "Введите логин пользователя: ";
      cin >> login;

      if (users.find(login) != users.end()) {
        cout << "Текущая роль: " << getRoleName(users[login].role) << endl;
        cout << "Новая роль (0 - Гость, 1 - Пользователь, 2 - Админ): ";
        cin >> newRoleChoice;

        Role newRole = Role::GUEST;
        if (newRoleChoice == 1)
          newRole = Role::USER;
        else if (newRoleChoice == 2)
          newRole = Role::ADMIN;

        users[login].role = newRole;
        cout << "Роль пользователя " << login << " изменена на "
             << getRoleName(newRole) << endl;
      } else {
        cout << "Пользователь не найден!" << endl;
      }

    } else if (choice == 4) {
      string login;
      cout << "Введите логин пользователя: ";
      cin >> login;

      if (users.find(login) != users.end()) {
        users[login].isActive = !users[login].isActive;
        cout << "Пользователь " << login << " теперь "
             << (users[login].isActive ? "активен" : "заблокирован") << endl;
      } else {
        cout << "Пользователь не найден!" << endl;
      }

    } else if (choice == 5) {
      break;
    } else {
      cout << "Неверный выбор!" << endl;
    }
  }
}

int main() {
  setlocale(LC_ALL, "Russian");

  cout << "Безопасный калькулятор с системой ролей\n" << endl;

  UserSession session = authenticate();

  if (!session.username.empty()) {
    if (hasPermission(session.role, Role::ADMIN)) {
      adminPanel(session);
    }
    calculator(session);
  } else {
    cout << "Программа завершена по соображениям безопасности." << endl;
    return 1;
  }

  return 0;
}
