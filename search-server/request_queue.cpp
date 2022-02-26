#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) :
        search_server_(search_server),
        count_empty_requests_(0),
        current_time_(0)
    {
    }

    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }

    vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int RequestQueue::GetNoResultRequests() const {
        return count_empty_requests_;
    }