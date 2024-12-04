#ifndef LOGGER_HPP
#define LOGGER_HPP

class Logger {
private:
    static void log_error(const char* notification, ...);
    static int solveChar(char word[], int ind, char kytu, int &sol);
    static void codeError(char word[], int sol);
public:
    static void errorStatus(int id);
};

#endif