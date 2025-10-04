#ifndef SECURITY_LOGGER_H
#define SECURITY_LOGGER_H

#include <sys/stat.h>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

class SecurityLogger {
 private:
  string logFilename;
  ofstream logFile;

  string getCurrentTimestamp() {
    time_t now = time(nullptr);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buffer);
  }

  void setFilePermissions() { chmod(logFilename.c_str(), S_IRUSR | S_IWUSR); }

 public:
  SecurityLogger(const string& filename = "../security.log")
      : logFilename(filename) {
    logFile.open(logFilename, ios::app);
    setFilePermissions();
  }

  ~SecurityLogger() {
    if (logFile.is_open()) {
      logFile.close();
    }
  }

  void logLoginSuccess(const string& username, const string& ip) {
    logFile << getCurrentTimestamp() << " [SUCCESS] Login: user='" << username
            << "' ip=" << ip << endl;
    logFile.flush();
  }

  void logLoginFailure(const string& username, const string& ip,
                       const string& reason) {
    logFile << getCurrentTimestamp() << " [FAILURE] Login: user='" << username
            << "' ip=" << ip << " reason='" << reason << "'" << endl;
    logFile.flush();
  }

  void logPasswordChange(const string& username, bool success) {
    logFile << getCurrentTimestamp() << " [PASSWORD] Change: user='" << username
            << "' success=" << (success ? "true" : "false") << endl;
    logFile.flush();
  }

  void logAdminAction(const string& adminUser, const string& action,
                      const string& target) {
    logFile << getCurrentTimestamp() << " [ADMIN] Action: admin='" << adminUser
            << "' action='" << action << "' target='" << target << "'" << endl;
    logFile.flush();
  }

  void logSecurityEvent(const string& event, const string& details) {
    logFile << getCurrentTimestamp() << " [SECURITY] " << event << ": "
            << details << endl;
    logFile.flush();
  }
};

#endif