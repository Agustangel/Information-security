#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>

using namespace std;

class SHA256 {
 public:
  static string hash(const string& data) {
    size_t hashValue = std::hash<string>{}(data + "salt123");
    stringstream ss;
    ss << hex << hashValue;
    return ss.str();
  }
};

map<string, string> users = {{"admin", SHA256::hash("password123")},
                             {"user1", SHA256::hash("qwerty")},
                             {"test", SHA256::hash("test")}};

bool isAdmin(const string& login) { return login == "admin"; }

void addUser(const string& login, const string& password) {
  users[login] = SHA256::hash(password);
  cout << "Пользователь " << login << " добавлен!" << endl;
}

string authenticate() {
  int attempts = 0;
  const int max_attempts = 3;
  string login, password;

  cout << "=== СИСТЕМА АУТЕНТИФИКАЦИИ ===" << endl;
  cout << "Максимальное количество попыток: " << max_attempts << endl;

  while (attempts < max_attempts) {
    cout << "Попытка " << (attempts + 1) << " из " << max_attempts << endl;
    cout << "Логин: ";
    cin >> login;
    cout << "Пароль: ";
    cin >> password;

    string hashedPassword = SHA256::hash(password);

    if (users.count(login) && users[login] == hashedPassword) {
      cout << "\n✓ Доступ разрешен! Добро пожаловать, " << login << "!" << endl;
      return login;
    } else {
      attempts++;
      int remaining = max_attempts - attempts;
      if (remaining > 0) {
        cout << "Неверные данные. Осталось попыток: " << remaining << endl;
      }
    }
  }

  cout << "\nДОСТУП ЗАБЛОКИРОВАН!" << endl;
  cout << "Превышено максимальное число попыток входа." << endl;
  return "";
}

void calculator() {
  char op;
  double num1, num2, result;

  cout << "\n" << string(50, '=') << endl;
  cout << "    НАУЧНЫЙ КАЛЬКУЛЯТОР" << endl;
  cout << string(50, '=') << endl;
  cout << "Поддерживаемые операции:" << endl;
  cout << "  +  - Сложение" << endl;
  cout << "  -  - Вычитание" << endl;
  cout << "  *  - Умножение" << endl;
  cout << "  /  - Деление" << endl;
  cout << "  %  - Остаток от деления" << endl;
  cout << "  q  - Выход" << endl;
  cout << string(50, '=') << endl;

  while (true) {
    cout << "\nВведите операцию (или 'q' для выхода): ";
    cin >> op;

    if (op == 'q' || op == 'Q') {
      cout << "Спасибо за использование калькулятора!" << endl;
      break;
    }

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
        cout << result << endl;
        break;
      case '-':
        result = num1 - num2;
        cout << result << endl;
        break;
      case '*':
        result = num1 * num2;
        cout << result << endl;
        break;
      case '/':
        if (num2 != 0) {
          result = num1 / num2;
          cout << result << endl;
        } else {
          cout << "ОШИБКА: Деление на ноль!" << endl;
        }
        break;
      case '%':
        if (num2 != 0) {
          result = (int)num1 % (int)num2;
          cout << result << endl;
        } else {
          cout << "ОШИБКА: Деление на ноль!" << endl;
        }
        break;
      default:
        cout << "ОШИБКА: Неподдерживаемая операция!" << endl;
    }
  }
}

void adminPanel() {
  int choice;
  while (true) {
    cout << "\n=== ПАНЕЛЬ АДМИНИСТРАТОРА ===" << endl;
    cout << "1. Добавить пользователя" << endl;
    cout << "2. Показать всех пользователей" << endl;
    cout << "3. Вернуться в калькулятор" << endl;
    cout << "Выберите опцию: ";

    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
      string login, password;
      cout << "Новый логин: ";
      cin >> login;
      cout << "Новый пароль: ";
      cin >> password;
      addUser(login, password);
    } else if (choice == 2) {
      cout << "Зарегистрированные пользователи:" << endl;
      for (const auto& user : users) {
        cout << "- " << user.first << endl;
      }
    } else if (choice == 3) {
      break;
    } else {
      cout << "Неверный выбор!" << endl;
    }
  }
}

int main() {
  setlocale(LC_ALL, "Russian");

  cout << "Безопасный калькулятор\n" << endl;

  string username = authenticate();
  if (!username.empty()) {
    if (isAdmin(username)) {
      adminPanel();
    }
    calculator();
  } else {
    cout << "Программа завершена по соображениям безопасности." << endl;
    return 1;
  }

  return 0;
}
