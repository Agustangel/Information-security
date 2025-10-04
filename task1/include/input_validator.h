#pragma once

#ifndef INPUT_VALIDATOR_H
#define INPUT_VALIDATOR_H

#include <iostream>
#include <limits>
#include <string>

using namespace std;

class InputValidator {
 public:
  static double getValidatedDouble(const string& prompt) {
    double value;
    while (true) {
      cout << prompt;
      if (cin >> value) {
        break;
      } else {
        cout << "Ошибка! Введите корректное число: ";
        cin.clear();
        clearInputBuffer();
      }
    }
    return value;
  }

  static int getValidatedInt(const string& prompt) {
    int value;
    while (true) {
      cout << prompt;
      if (cin >> value) {
        break;
      } else {
        cout << "Ошибка! Введите целое число: ";
        cin.clear();
        clearInputBuffer();
      }
    }
    return value;
  }

  static int getMenuChoice(int minChoice, int maxChoice) {
    int choice;
    while (true) {
      cout << "Выберите опцию: ";
      if (cin >> choice && choice >= minChoice && choice <= maxChoice) {
        break;
      } else {
        cout << "Неверный выбор! Введите число от " << minChoice << " до "
             << maxChoice << ": ";
        cin.clear();
        clearInputBuffer();
      }
    }
    return choice;
  }

  static void clearInputBuffer() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }
};

#endif