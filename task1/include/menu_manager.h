#pragma once

#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include "auth_manager.h"
#include "calculator_engine.h"
#include "password_policy.h"

using namespace std;

class MenuManager {
 private:
  UserDatabase& userDB;
  SecurityLogger& securityLogger;
  PasswordPolicy& passwordPolicy;
  AuthManager& authManager;
  CalculatorEngine& calculatorEngine;

  void displayCalculatorMenu(const UserSession& session);
  void handleBasicOperations(char op, const UserSession& session);
  void handleAdvancedOperations(char op, const UserSession& session);
  bool validatePermission(const UserSession& session, char operation);

 public:
  MenuManager(UserDatabase& db, SecurityLogger& logger, PasswordPolicy& policy,
              AuthManager& auth, CalculatorEngine& calc);

  void showCalculator(const UserSession& session);
  void showAdminPanel(UserSession& session);
  void showChangePassword(const UserSession& session);
  void showUserMenu(UserSession& session);
};

#endif