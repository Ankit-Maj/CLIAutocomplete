// autocomplete.cpp
// Windows-only real-time CLI autocomplete (trimmed for CP-style).
// Requires GCC/MinGW (bits/stdc++.h). For MSVC replace bits header with specific includes.
//
// Build (MinGW / g++):
//   g++ -std=c++17 -O2 -o autocomplete.exe autocomplete.cpp
// Run:
//   autocomplete.exe words.txt [K]
//
// Controls:
//  - Type characters: live suggestions update
//  - Backspace: delete last char
//  - Tab: accept current selection (or top suggestion if none selected)
//  - Arrow Up / Arrow Down: navigate suggestions
//  - Enter: select buffer (prints "Selected: <word>") and reset
//  - Esc: exit

#include <bits/stdc++.h>
#include <windows.h>
#include <conio.h>

using namespace std;

// ----------------- Load words -----------------
bool load_words(const string& path, vector<pair<string,int>>& out) {
    ifstream in(path);
    if (!in) return false;
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string w;
        long long freqll = 0;
        if (!(iss >> w)) continue;
        if (!(iss >> freqll)) freqll = 0;
        if (freqll > INT_MAX) freqll = INT_MAX;
        out.emplace_back(w, static_cast<int>(freqll));
    }
    sort(out.begin(), out.end(), [](const pair<string,int>& a, const pair<string,int>& b){
        return a.first < b.first;
    });
    return true;
}

// ----------------- Autocomplete (basic fast) -----------------
// returns up to k pairs (word,freq) sorted by freq desc, then word asc
vector<pair<string,int>> autocomplete(const vector<pair<string,int>>& words, const string& prefix, int k) {
    vector<pair<string,int>> res;
    if (prefix.empty() || words.empty() || k <= 0) return res;

    auto comp_elem_vs_str = [](const pair<string,int>& a, const string& b){
        return a.first < b;
    };
    string prefix_upper = prefix;
    prefix_upper.push_back(char(127)); // sentinel

    auto it_low = lower_bound(words.begin(), words.end(), prefix, comp_elem_vs_str);
    auto it_up  = lower_bound(words.begin(), words.end(), prefix_upper, comp_elem_vs_str);
    if (it_low == it_up) return res;

    struct Node { int freq; const string* w; };
    struct Cmp {
        bool operator()(const Node& a, const Node& b) const {
            if (a.freq != b.freq) return a.freq > b.freq; // min-heap by freq
            return *(a.w) < *(b.w);
        }
    };
    priority_queue<Node, vector<Node>, Cmp> pq;
    for (auto it = it_low; it != it_up; ++it) {
        Node n{it->second, &it->first};
        if ((int)pq.size() < k) pq.push(n);
        else {
            const Node& top = pq.top();
            if (n.freq > top.freq || (n.freq == top.freq && *(n.w) < *(top.w))) {
                pq.pop();
                pq.push(n);
            }
        }
    }
    while (!pq.empty()) {
        res.emplace_back(*pq.top().w, pq.top().freq);
        pq.pop();
    }
    reverse(res.begin(), res.end());
    return res;
}

// ----------------- Console / ANSI helpers (Windows) -----------------
bool enable_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return false;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(hOut, dwMode)) return false;
    return true;
}

void move_cursor_up(int n) {
    if (n <= 0) return;
    cout << "\x1b[" << n << "A";
}
void clear_line() {
    cout << "\x1b[2K";
}
void carriage_return() {
    cout << "\r";
}

void erase_prev_block(int prev_lines) {
    if (prev_lines <= 0) return;
    move_cursor_up(prev_lines);
    for (int i = 0; i < prev_lines; ++i) {
        clear_line();
        cout << "\n";
    }
    move_cursor_up(prev_lines);
}

int render_ui(const string& prompt_prefix, const string& buffer, const vector<pair<string,int>>& suggestions, int selected_index) {
    clear_line();
    carriage_return();
    cout << prompt_prefix << buffer << " ";
    cout << "\n";
    for (size_t i = 0; i < suggestions.size(); ++i) {
        clear_line();
        carriage_return();
        if ((int)i == selected_index) cout << "\x1b[7m";
        cout << "  " << (i+1) << ". " << suggestions[i].first << " (" << suggestions[i].second << ")";
        if ((int)i == selected_index) cout << "\x1b[0m";
        cout << "\n";
    }
    cout.flush();
    return 1 + (int)suggestions.size();
}

// ----------------- Main -----------------
int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " words.txt [K]" << endl;
        return 1;
    }
    string words_file = argv[1];
    int K = 5;
    if (argc >= 3) {
        try { K = stoi(argv[2]); } catch(...) { K = 5; }
        if (K <= 0) K = 5;
    }

    vector<pair<string,int>> words;
    if (!load_words(words_file, words)) {
        cerr << "Failed to load words from '" << words_file << "'." << endl;
        cerr << "Expected file format: one line per entry: word [frequency]" << endl;
        return 1;
    }
    cout << "Loaded " << words.size() << " entries. Top-K = " << K << endl << endl;

    if (!enable_ansi()) {
        cerr << "Warning: failed to enable ANSI escape sequences. UI may not render correctly." << endl;
    }

    cout << "Real-time Autocomplete (Windows - trimmed)\n";
    cout << "Type to see suggestions. Tab: accept, Enter: select, Esc: exit. Arrow keys to navigate.\n\n";

    string buffer;
    int prev_lines = 0;
    vector<pair<string,int>> suggestions;
    int selected = -1;

    prev_lines = render_ui("> ", buffer, suggestions, selected);

    bool running = true;
    while (running) {
        int c = _getch();
        if (c == 0 || c == 224) {
            int c2 = _getch();
            if (c2 == 72) { // up
                if (!suggestions.empty()) {
                    if (selected <= 0) selected = (int)suggestions.size() - 1;
                    else --selected;
                }
            } else if (c2 == 80) { // down
                if (!suggestions.empty()) {
                    if (selected < 0) selected = 0;
                    else selected = (selected + 1) % (int)suggestions.size();
                }
            }
        } else if (c == 9) { // Tab
            if (!suggestions.empty()) {
                if (selected >= 0) buffer = suggestions[selected].first;
                else buffer = suggestions.front().first;
            }
        } else if (c == 8) { // Backspace
            if (!buffer.empty()) buffer.pop_back();
        } else if (c == 13) { // Enter
            erase_prev_block(prev_lines);
            cout << "Selected: " << buffer << "\n\n";
            buffer.clear();
            suggestions.clear();
            selected = -1;
            prev_lines = render_ui("> ", buffer, suggestions, selected);
            continue;
        } else if (c == 27) { // Esc
            running = false;
            break;
        } else if (c >= 32 && c <= 126) { // printable ASCII
            buffer.push_back((char)c);
        } else {
            // ignore other control chars
        }

        suggestions = autocomplete(words, buffer, K);
        if (suggestions.empty()) selected = -1;
        else {
            if (selected >= (int)suggestions.size()) selected = (int)suggestions.size() - 1;
            if (selected < 0) selected = 0;
        }

        erase_prev_block(prev_lines);
        prev_lines = render_ui("> ", buffer, suggestions, selected);
    }

    erase_prev_block(prev_lines);
    cout << "Exiting.\n";
    return 0;
}
