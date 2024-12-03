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
    // Sort for efficient searching
    std::sort(URLs.begin(), URLs.end());
    for (auto x: URLs) std::cerr << x << "\n";
}

bool BlackList::isMember(const string &host) {
    // Perform a binary search for efficiency
    return std::binary_search(URLs.begin(), URLs.end(), host);
}