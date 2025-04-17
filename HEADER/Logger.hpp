#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <unordered_map>
#include <mutex>

class Logger {
private:
    const std::string LOG_FILE="proxy_errors.log";
    
    std::unordered_map<int, std::string> errorMessages;
    std::string errorFilePath;
    std::mutex logMutex;

    std::string getCurrentTime();
    void loadErrorMessages();
public:
    Logger(const std::string &path);
    void logError(int errorCode);
    void logMessage(const std::string& message);
    void addCustomError(int errorCode, const std::string& errorMessage);
};

extern Logger logger;

#endif