#include <vector>
#include <string>
#include <algorithm>
#include <execution>

#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
	vector<vector<Document>> res(queries.size());
	transform(execution::par, queries.begin(), queries.end(), res.begin(), 
		[&](const string& query) {return search_server.FindTopDocuments(query); });
	return res;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
	vector<Document> res;
	vector<vector<Document>> documents = ProcessQueries(search_server, queries);
	for (auto& doc : documents) {
		res.insert(res.end(), make_move_iterator(doc.begin()), make_move_iterator(doc.end()) );
	}
	return res;
}