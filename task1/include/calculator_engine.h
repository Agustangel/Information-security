#pragma once

#ifndef CALCULATOR_ENGINE_H
#define CALCULATOR_ENGINE_H

#include <cmath>
#include <stdexcept>

#include "auth_manager.h"

using namespace std;

class CalculatorEngine {
 public:
  static long long factorial(int n);
  static double power(double base, double exponent);

  struct CalculationResult {
    bool success;
    double value;
    string errorMessage;
  };

  CalculationResult calculate(char operation, double num1, double num2 = 0);
  CalculationResult calculateAdvanced(char operation, double num);
};

#endif