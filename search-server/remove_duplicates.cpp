#include "remove_duplicates.h"

#include <vector>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> dup_ids;
    map<std::string, double> w1;

    for (const int document_id : search_server) {
        w1 = search_server.GetWordFrequencies(document_id); // список слов в текущем документе

        for (const int doc_id : search_server) {
            if (doc_id > document_id) {
                auto w2 = search_server.GetWordFrequencies(doc_id); // список слов в другом документе
                auto it = w2.begin();
                if (w1.size() == w2.size()) {
                    for ( ; it != w2.end(); it++) { // проверка наличия слова из другого документа в текущем
                        if(w1.find(it->first) == w1.end()) {
                            break;
                        }
                    }
                    if (it == w2.end()) { // дубликат
                        dup_ids.insert(doc_id); // ids дубликатов
                    }
                }
            }

        }
    }

    // удаление
    for (int id : dup_ids) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id " << id << endl;
    }
}