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
    // if (id == -1) Logger::log_error("MISSING HEADER HTTP DATA");
    // else if (id == -2) Logger::log_error("HOST HEADER NOT FOUND IN REQUEST");
    // else if (id == -3) Logger::log_error("INVALID PORT NUMBER.");
    // else if (id == -4) Logger::log_error("PORT NUMBER OUT OF RANGE.");
    // else if (id == -5) Logger::log_error("HOST NOT FOUND IN REQUEST");
    // else if (id == -6) Logger::log_error("SKIP");
    // else if (id == -7) Logger::log_error("BLOCKED! -> HOST");
    // else if (id == -8) Logger::log_error("FAILED TO SEND RESPONSE.");
    // else if (id == -9) Logger::log_error("CANNOT CONNECT TO BROWSER.");
    // else if (id == -10) Logger::log_error("FAILED TO RESOLVE HOST NAME");
    // else if (id == -11) Logger::log_error("CANNOT CREATE REMOTE SOCKET");
    // else if (id == -12) Logger::log_error("ERROR CONNECTING TO REMOTE SERVER");
    // else if (id == -13) Logger::log_error("WSAPOLL ERROR");
    // else if (id == -14) Logger::log_error("BROWSER/REMOTE HAVE SOME PROBLEMS");
    // else if (id == -15) Logger::log_error("SENDING ERRORS");
    // else if (id == -16) Logger::log_error("CONNECTION CLOSED");
    // else if (id == -17) Logger::log_error("RECEIVING ERRORS");
    // else if (id == -18) Logger::log_error("ERROR CREATING SERIAL NUMBER");
    // else if (id == -19) Logger::log_error("ERROR GENERATING RANDOM SERIAL NUMBER");    
    // else if (id == -20) Logger::log_error("ERROR GENERATING SERIAL NUMBER");
    // else if (id == -21) Logger::log_error("CANNOT OPEN ROOT KEY OR ROOT CERTIFICATE FILES");
    // else if (id == -22) Logger::log_error("ERROR LOADING ROOT KEY OR ROOT CERTIFICATE");
    // else if (id == -23) Logger::log_error("ERROR SIGNING CERTIFICATE");
    // else if (id == -24) Logger::log_error("CANNOT OPEN OUTPUT FILES FOR WRITING");
    // else if (id == -25) Logger::log_error("FAILED TO CREATE DIRECTORY");
    // else if (id == -26) Logger::log_error("FAILED TO GENERATE CERTIFICATE");
    // else if (id == -27) Logger::log_error("SSL CERTIFICATE VERIFICATION FAILED");
    // else if (id == -28) Logger::log_error("CANNOT ACCEPT cLIENTSSL");
    // else if (id == -29) Logger::log_error("CANNOT CREATE SSL FOR SERVER");
    // else if (id == -30) Logger::log_error("CANNOT CREATE SSL OBJECT");
    // else if (id == -31) Logger::log_error("FAILED TO SET SNI (SERVER NAME INDICATION)");
    // else if (id == -32) Logger::log_error("SSL_cONNECT FAILED WITH ERROR CODE");
    // else if (id == -33) Logger::log_error("WSASTARTUP FAILED");
    // else if (id == -34) Logger::log_error("SOCKET CREATION FAILED");
    // else if (id == -35) Logger::log_error("SENT MESSAGE FAILED");
    // else if (id == -36) Logger::log_error("ERROR RECEIVING DATA");
    // else if (id == -37) Logger::log_error("CONNECTION INTERRUPTED");
    // else if (id == -38) Logger::log_error("ERROR SENDING DATA");
    // else if (id == -39) Logger::log_error("CONNECTION CLOSED BY PEER");
    // else if (id == -40) Logger::log_error("GETADAPTERSADDRESSES FAILED");
    // else if (id == -41) Logger::log_error("BIND FAILED");
    // else if (id == -42) Logger::log_error("IN PROXY SERVER END");
    // else if (id == -43) Logger::log_error("INTERRUPTED SIGNAL");
    // else if (id == -44) Logger::log_error("LISTEN FAILED");
    // else if (id == -45) Logger::log_error("ERROR ACCEPTING CONNECTION FROM CLIENT");
    // else if (id == -46) Logger::log_error("EXCEPTION IN TASK");
    // else if (id == -47) Logger::log_error("UNKNOWN EXCEPTION IN TASK");

    const char pathErrorNameRule[] = "./ErrorName_Rule.txt";
    FILE* f = fopen(pathErrorNameRule, "r");

    char word[50] = ""; int sol = 0;
    do {
        if (word[0] == '-' && solveChar(word, 1, ':', sol) == id) Logger::codeError(word, sol + 2);
    } while (fgets(word, 50, f));
}
