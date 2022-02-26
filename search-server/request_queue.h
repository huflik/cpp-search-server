#pragma once

#include "search_server.h"

#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate); 

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        bool empty_request_;
        uint64_t timestamp;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int count_empty_requests_;
    uint64_t current_time_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> request = search_server_.FindTopDocuments(raw_query, document_predicate);
    ++current_time_;
    if (request.empty()) {
        ++count_empty_requests_;
        requests_.push_back({ true, current_time_ });
    }
    else {
        requests_.push_back({ false, current_time_ });
    }

    if (min_in_day_ <= current_time_ - requests_.front().timestamp) {
        if (requests_.front().empty_request_) {
            --count_empty_requests_;
        }
        requests_.pop_front();
    }
    return request;
}