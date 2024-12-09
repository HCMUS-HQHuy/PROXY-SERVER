#include <fstream>
#include <iostream>
#include <algorithm>
#include "./../../HEADER/BlackList.hpp"

BlackList blackList("./CONFIG/blocked_sites.txt");

BlackList::BlackList(const string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open blacklist file: " << path << '\n';
        return;
    }

    string url;
    while (std::getline(file, url)) {
        url.erase(url.find_last_not_of(" \t\r\n") + 1);
        if (!url.empty()) {
            URLs.push_back(url);
        }
    }

    file.close();
    // for (auto x: URLs) std::cerr << x << "\n";
}

bool BlackList::isMember(const string &host) {
    // Duyệt qua từng URL trong danh sách
    for (const auto &url : URLs) {
        // Kiểm tra nếu URL là phần cuối của host
        if (host.length() >= url.length() && 
            host.compare(host.length() - url.length(), url.length(), url) == 0) {
            return true; // Host bị chặn
        }
    }
    return false; // Host không bị chặn
}