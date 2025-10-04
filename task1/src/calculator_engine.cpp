#include "calculator_engine.h"

#include <cmath>
#include <stdexcept>

using namespace std;

long long CalculatorEngine::factorial(int n) {
  long long result = 1;
  for (int i = 2; i <= n; ++i) {
    result *= i;
  }
  return result;
}

double CalculatorEngine::power(double base, double exponent) {
  return pow(base, exponent);
}

CalculatorEngine::CalculationResult CalculatorEngine::calculate(char operation,
                                                                double num1,
                                                                double num2) {
  CalculationResult result;

  try {
    switch (operation) {
      case '+':
        result.value = num1 + num2;
        break;
      case '-':
        result.value = num1 - num2;
        break;
      case '*':
        result.value = num1 * num2;
        break;
      case '/':
        if (num2 == 0) throw runtime_error("Деление на ноль!");
        result.value = num1 / num2;
        break;
      case '^':
        result.value = power(num1, num2);
        break;
      default:
        throw runtime_error("Неподдерживаемая операция!");
    }
    result.success = true;
  } catch (const exception& e) {
    result.success = false;
    result.errorMessage = e.what();
  }

  return result;
}

CalculatorEngine::CalculationResult CalculatorEngine::calculateAdvanced(
    char operation, double num) {
  CalculationResult result;

  try {
    switch (operation) {
      case '!':
        if (num < 0)
          throw runtime_error("Факториал отрицательного числа не определен!");
        if (num > 20)
          throw runtime_error("Слишком большое число для факториала!");
        result.value = static_cast<double>(factorial(static_cast<int>(num)));
        break;
      case 's':
        if (num < 0)
          throw runtime_error("Квадратный корень из отрицательного числа!");
        result.value = sqrt(num);
        break;
      case 'l':
        if (num <= 0)
          throw runtime_error(
              "Логарифм определен только для положительных чисел!");
        result.value = log(num);
        break;
      default:
        throw runtime_error("Неподдерживаемая операция!");
    }
    result.success = true;
  } catch (const exception& e) {
    result.success = false;
    result.errorMessage = e.what();
  }

  return result;
}