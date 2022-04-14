#include "remove_duplicates.h"

#include <vector>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    
    set<int> dup_ids;
    set<set<string_view>> uniqs;

    for (const int document_id : search_server) {
        set<string_view> cstr;
        auto w1 = search_server.GetWordFrequencies(document_id);

        for (auto word = w1.begin(); word != w1.end(); word++) {
            cstr.insert(word->first);
        }
        if (uniqs.find(cstr) == uniqs.end()) {
            uniqs.insert(cstr);
        }
        else {
            dup_ids.insert(document_id);
        }
    }
    // удаление
    for (int id : dup_ids) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id " << id << endl;
    }
}