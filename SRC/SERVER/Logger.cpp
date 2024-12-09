#include<stdio.h>
#include "./../../HEADER/Logger.hpp"
#include "./../../HEADER/Setting.hpp"

#define LOG_FILE "proxy_errors.log"

// Function to write log error into log file
void Logger::log_error(const char* notification, ...) {
    FILE* log_file = fopen(LOG_FILE, "a");

    if (!log_file) {
        perror("Cannot open log file\n");
        return;
    }

    time_t now = time(NULL);
    struct tm* local_time = localtime(&now);
    if (!local_time) {
        perror("Cannot get the system time\n");
        fclose(log_file);
        return;
    }

    fprintf(log_file, "[%d-%d-%d %d:%d:%d] ERROR: ", local_time->tm_mday, local_time->tm_mon, local_time->tm_year, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    va_list args;
    va_start(args, notification);
    vfprintf(log_file, notification, args);
    va_end(args);

    fprintf(log_file, "\n");

    fclose(log_file);
}

int Logger::solveChar(char word[], int ind, char kytu, int &sol) {
    int result = 0;
    while (word[ind] != kytu) {
        result = result * 10 - (word[ind] - 48);
        ind++;
    }
    sol = ind;

    return result;
}

void Logger::codeError(char word[], int sol) {
    char sub[50] = "";
    for (int i = sol; word[i] != '\n'; i++) sub[i - sol] = word[i];

    Logger::log_error(sub);
}

void Logger::errorStatus(int id) {
    const char pathErrorNameRule[] = "./ErrorName_Rule.txt";
    FILE* f = fopen(pathErrorNameRule, "r");

    char word[50] = ""; int sol = 0;
    do {
        if (word[0] == '-' && solveChar(word, 1, ':', sol) == id) Logger::codeError(word, sol + 2);
    } while (fgets(word, 50, f));
}
