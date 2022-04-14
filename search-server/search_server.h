#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>
#include <cmath>
#include <iostream>
#include <numeric>
#include <execution>
#include <functional>
#include <future>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "log_duration.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_COMPARISON_ERR = 1e-6;


class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, DocumentPredicate document_predicate) const; 
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy ex_policy, std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy ex_policy, std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy ex_policy, std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy ex_policy, std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy ex_policy, std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::vector<int>::const_iterator begin() const;
    std::vector<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy ex_policy, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy ex_policy, const std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id);
    
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy ex_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy ex_policy, int document_id);


private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string content;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> document_ids_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string_view text) const;
    Query ParseQuery(std::string_view text, bool skip_sort = false) const;

    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const; 
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy ex_policy, const Query& query, DocumentPredicate document_predicate) const; 
    
    template <typename DocumentPredicate>
    void FindAllDocumentsConcurrent(const std::string_view& word, DocumentPredicate document_predicate, ConcurrentMap<int, double>& document_to_relevance) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using namespace std::string_literals;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_COMPARISON_ERR) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy ex_policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_COMPARISON_ERR) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const {
    constexpr size_t TASK_COUNT = 4;
    constexpr size_t THREAD_COUNT = 100;
    ConcurrentMap<int, double> document_to_relevance(THREAD_COUNT);
    std::vector<std::future<void>> futures;
    size_t part_range_size = query.plus_words.size() / TASK_COUNT;
    size_t range_teil = query.plus_words.size() % TASK_COUNT;
    auto end_range = range_teil ? prev(query.plus_words.end(), range_teil) : query.plus_words.end(); 

    if (part_range_size) {
        for (auto it = query.plus_words.begin(); it != end_range; advance(it, part_range_size)) {
            futures.push_back(std::async([=, &document_to_relevance]() {
                for (auto word_it = it; word_it != next(it, part_range_size); ++word_it) {
                    FindAllDocumentsConcurrent(*word_it, document_predicate, document_to_relevance);
                }
                }));
        }
    }
    if (range_teil) {
        for (auto word_it = end_range; word_it != query.plus_words.end(); ++word_it) {
            FindAllDocumentsConcurrent(*word_it, document_predicate, document_to_relevance);
        }
    }
    for (auto& f : futures) {
        f.get();
    }
    auto result = document_to_relevance.BuildOrdinaryMap();
    for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(word) != 0) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                result.erase(document_id);
            }
        }
        });
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : result) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}

template <typename DocumentPredicate>
void SearchServer::FindAllDocumentsConcurrent(const std::string_view& word, DocumentPredicate document_predicate, ConcurrentMap<int, double>& document_to_relevance) const {
    if (word_to_document_freqs_.count(word) != 0) {
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    }
}