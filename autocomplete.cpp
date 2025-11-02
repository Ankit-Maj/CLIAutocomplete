
#include <bits/stdc++.h>
using namespace std;

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

// returns up to k pairs (word,freq) sorted by freq desc, then word asc
vector<pair<string,int>> autocomplete(const vector<pair<string,int>>& words, const string& prefix, int k) {
    vector<pair<string,int>> res;
    if (prefix.empty() || words.empty() || k <= 0) return res;

    auto comp_elem_vs_str = [](const pair<string,int>& a, const string& b){
        return a.first < b;
    };
    string prefix_upper = prefix;
    prefix_upper.push_back(char(127)); 
    auto it_low = lower_bound(words.begin(), words.end(), prefix, comp_elem_vs_str);
    auto it_up  = lower_bound(words.begin(), words.end(), prefix_upper, comp_elem_vs_str);
    if (it_low == it_up) return res;

    struct Node { int freq; const string* w; };
    struct Cmp {
        bool operator()(const Node& a, const Node& b) const {
            if (a.freq != b.freq) return a.freq > b.freq;
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

    cout << "Loaded " << words.size() << " entries. Top-K = " << K << "\n\n";

    while (true) {
        cout << "Enter prefix (or 'exit' to quit): ";
        string prefix;
        if (!getline(cin, prefix)) break;
        if (prefix == "exit") break;

        vector<pair<string,int>> suggestions = autocomplete(words, prefix, K);
        if (suggestions.empty()) {
            cout << "No matches found.\n\n";
            continue;
        }

        cout << "Suggestions:\n";
        for (size_t i = 0; i < suggestions.size(); ++i)
            cout << "  " << (i+1) << ". " << suggestions[i].first << " (" << suggestions[i].second << ")\n";
        cout << "\n";
    }

    cout << "Goodbye!\n";
    return 0;
}
