#include <fstream>
#include <iostream>
#include "./../../HEADER/BlackList.hpp"

BlackList blackList("./CONFIG/blocked_sites.txt");

BlackList::BlackList(const string &path) {
    std::fstream fi(path, std::ios::in);
    if (fi.is_open() == false) {
        std::cerr << "CANNOT OPEN FILE\n";
    }
    std::string buffer;
    while (std::getline(fi, buffer)) {
        URLs.push_back(buffer);
    }
    fi.close();
    for (auto x: URLs) std::cerr << x << "\n";

}


bool BlackList::isMember(const string &host) {
    for (auto x: URLs) 
        if (x == host)
            return true;
    return false;
}