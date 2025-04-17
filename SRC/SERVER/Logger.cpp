#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include "./../../HEADER/UI.hpp"
#include "./../../HEADER/Logger.hpp"

Logger logger("./CONFIG/ErrorName_Rule.txt");

Logger::Logger(const std::string &path):errorFilePath(path) {
    std::ofstream logFile(LOG_FILE, std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not clear log file: " << LOG_FILE << "\n";
    }
    logFile.close();
    loadErrorMessages();
}

void Logger::loadErrorMessages() {
    errorMessages.clear();
    std::ifstream errorFile(errorFilePath);
    if (!errorFile.is_open()) {
        std::cerr << "ERROR: Could not open error file: " << errorFilePath << "\n";
        return;
    }

    std::string line;
    while (std::getline(errorFile, line)) {
        if (line.empty() || line[0] == '/') continue; // Bỏ qua dòng trống và comment

        std::istringstream iss(line);
        int errorCode;
        std::string errorMessage;

        if (iss >> errorCode && std::getline(iss, errorMessage)) {
            // Loại bỏ khoảng trắng đầu nếu có
            if (!errorMessage.empty() && (errorMessage[0] == ' ' || errorMessage[0] == ':')) {
                errorMessage.erase(0, 1);
            }
            errorMessages[errorCode] = errorMessage;
        }
    }
    errorFile.close();
}

// Hàm lấy thời gian hiện tại
std::string Logger::getCurrentTime() {
    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

void Logger::logError(int errorCode) {
    std::lock_guard<std::mutex> lock(logMutex); // Đảm bảo thread-safe

    std::wofstream logFile(LOG_FILE, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not open log file: " << LOG_FILE << "\n";
        return;
    }

    std::string errorMessage = "Unknown error";
    if (errorMessages.find(errorCode) != errorMessages.end()) {
        errorMessage = errorMessages[errorCode];
    }

    std::wstringstream buf;
    buf << "[" << getCurrentTime().c_str() << "] ERROR: " << errorMessage.c_str() << "\r\n";

    logFile << buf.str();
    Window.AppendEdit(buf.str());
    logFile.close();
}

// Hàm log thông thường
void Logger::logMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex); // Đảm bảo thread-safe

    std::wofstream logFile(LOG_FILE, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not open log file: " << LOG_FILE << "\r\n";
        return;
    }

    std::wstringstream buf;
    buf << "[" << getCurrentTime().c_str() << "] " << message.c_str() << "\r\n";
    logFile << buf.str();
    Window.AppendEdit(buf.str());
    logFile.close();
}

// Hàm thêm lỗi tùy chỉnh
void Logger::addCustomError(int errorCode, const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(logMutex);
    errorMessages[errorCode] = errorMessage;
}