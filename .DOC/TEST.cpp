#include <iostream>
#include <string>
#include <stack>

// ah chừ 1 dấu nè

int main() {
    std::string s; std::cin >> s;
    std::stack<char> st;
    bool ok = true;
    for (int i = 0; i < s.size(); i++) {
        if (s[i] == '(' || s[i] == '[' || s[i] == '{') {
            st.push(s[i]);
        }
        if (s[i] == ')' || s[i] == ']' || s[i] == '}') {
            if (s[i] == st.top()) {
                ok = false; break;
            }
            else st.pop();
        }
    }
    if (ok) std::cout << "OK!\n";
    else std::cout << "NO\n";
    return 0;
}