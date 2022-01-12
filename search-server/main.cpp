// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
// Тест добавления документа и нахождение его по словам из запроса
void TestAddDocumentAndFindByQuery() {
    const int doc_id = 25;
    const string content = "the cat from the house of the mouse to the dance"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Убеждаемся, что поиск слова,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("house from mouse"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Если слов из запроса в документе нет возвращается пустой результат
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("hole"s);
        ASSERT(server.FindTopDocuments("hole"s).empty());
    }
}

// Тест проверяет, что поисковая система исключает из выдачи документы с минус-словами
void TestExcludeDocumentsWithMinusWords() {
    const int doc_id = 25;
    const string content = "the cat from the white house of the mouse to the dance"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Сначала убеждаемся, что документ без минус-слова выдается
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("house from mouse"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    // Проверяем, что документ с минус словом не выдается
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("house from mouse -white"s);
        ASSERT_HINT(server.FindTopDocuments("hole"s).empty(), "Documents with minus-words must be excluded from result"s);
    }
}

// Тест проверяет, что при матчнге возвращаются все слова из запроса, присутствующие в документе
void TestMatchedWords() {
    const int doc_id = 25;
    const string content = "the cat from the white house of the mouse to the dance"s;
    const vector<int> ratings = { 1, 2, 3 };
    // Убеждаемся, что выдаются все слова из запроса, присутствующие в документе 
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto result = server.MatchDocument("house from mouse blue"s, doc_id);
        vector<string> words = { "from"s, "house"s, "mouse"s };
        ASSERT_EQUAL(get<vector<string>>(result), words);
    }
    // Проверяем, что если есть минус слово возвращается пустой результат 
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto result = server.MatchDocument("house from mouse blue -white"s, doc_id);
        ASSERT(get<vector<string>>(result).empty());
    }
}
// Тест проверяет, что документы при выдаче выдаются по убыванию релевантности
void TestSortByRelevanceDocuments() {
    const vector<int> doc_id = { 25 , 26, 27, 28 };
    const vector<string> content = { "the cat from the white house of the mouse to the dance"s,
                                     "the mouse like dance and chees"s,
                                     "the cat go from blue house"s,
                                     "the mouse eat blue chees and dance"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },  { 5, -12, 2, 1 }, { 9 } };

    {
        SearchServer server;
        server.AddDocument(doc_id[0], content[0], DocumentStatus::ACTUAL, ratings[0]);
        server.AddDocument(doc_id[1], content[1], DocumentStatus::ACTUAL, ratings[1]);
        server.AddDocument(doc_id[2], content[2], DocumentStatus::ACTUAL, ratings[2]);
        server.AddDocument(doc_id[3], content[3], DocumentStatus::ACTUAL, ratings[3]);
        ostringstream oss;
        for (const Document& document : server.FindTopDocuments("house from mouse"s)) {
            oss << document.id << endl;
        }
        ASSERT_EQUAL(oss.str(), "27\n25\n26\n28\n");
    }
}

// Тест проверяет, как считается средний рейтинг
void TestCheckRatingDocuments() {
    const vector<int> doc_id = { 25 , 26, 27, 28 };
    const vector<string> content = { "the cat from the white house of the mouse to the dance"s,
                                     "the mouse like dance and chees"s,
                                     "the cat go from blue house"s,
                                     "the mouse eat blue chees and dance"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },  { 5, -12, 2, 1 }, { 9 } };
    {
        SearchServer server;
        server.AddDocument(doc_id[0], content[0], DocumentStatus::ACTUAL, ratings[0]);
        server.AddDocument(doc_id[1], content[1], DocumentStatus::ACTUAL, ratings[1]);
        server.AddDocument(doc_id[2], content[2], DocumentStatus::ACTUAL, ratings[2]);
        server.AddDocument(doc_id[3], content[3], DocumentStatus::ACTUAL, ratings[3]);
        ostringstream oss;
        for (const Document& document : server.FindTopDocuments("house from mouse"s, DocumentStatus::ACTUAL)) {
            oss << document.rating << endl;
        }
        ASSERT_EQUAL(oss.str(), "-1\n2\n5\n9\n");
    }
}
// Тест проверяет, использование функции предиката для фильтрации
void TestUserFilterDocuments() {
    const vector<int> doc_id = { 25 , 26, 27, 28 };
    const vector<string> content = { "the cat from the white house of the mouse to the dance"s,
                                     "the mouse like dance and chees"s,
                                     "the cat go from blue house"s,
                                     "the mouse eat blue chees and dance"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },  { 5, -12, 2, 1 }, { 9 } };
    {
        SearchServer server;
        server.AddDocument(doc_id[0], content[0], DocumentStatus::ACTUAL, ratings[0]);
        server.AddDocument(doc_id[1], content[1], DocumentStatus::ACTUAL, ratings[1]);
        server.AddDocument(doc_id[2], content[2], DocumentStatus::ACTUAL, ratings[2]);
        server.AddDocument(doc_id[3], content[3], DocumentStatus::ACTUAL, ratings[3]);
        {
            ostringstream oss;
            for (const Document& document : server.FindTopDocuments("house from mouse"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
                oss << document.id << endl;
            }
            ASSERT_EQUAL(oss.str(), "26\n28\n");
        }

    }
}
// Тест проверяет, как ищется документ с заданным статусом
void TestCheckStatusDocuments() {
    const vector<int> doc_id = { 25 , 26, 27, 28 };
    const vector<string> content = { "the cat from the white house of the mouse to the dance"s,
                                     "the mouse like dance and chees"s,
                                     "the cat go from blue house"s,
                                     "the mouse eat blue chees and dance"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },  { 5, -12, 2, 1 }, { 9 } };
    {
        SearchServer server;
        server.AddDocument(doc_id[0], content[0], DocumentStatus::ACTUAL, ratings[0]);
        server.AddDocument(doc_id[1], content[1], DocumentStatus::BANNED, ratings[1]);
        server.AddDocument(doc_id[2], content[2], DocumentStatus::IRRELEVANT, ratings[2]);
        server.AddDocument(doc_id[3], content[3], DocumentStatus::REMOVED, ratings[3]);
        {
            const auto found_docs = server.FindTopDocuments("house from mouse"s);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id[0]);
        }
        {
            const auto found_docs = server.FindTopDocuments("house from mouse"s, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id[1]);
        }
        {
            const auto found_docs = server.FindTopDocuments("house from mouse"s, DocumentStatus::IRRELEVANT);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id[2]);
        }
        {
            const auto found_docs = server.FindTopDocuments("house from mouse"s, DocumentStatus::REMOVED);
            ASSERT_EQUAL(found_docs.size(), 1u);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_id[3]);
        }

    }
}
// Тест проверяет корректность вычисления релевантности
void TestCheckRelevanceDocuments() {
    const vector<int> doc_id = { 25 , 26, 27, 28 };
    const vector<string> content = { "the cat from the white house of the mouse to the dance"s,
                                     "the mouse like dance and chees"s,
                                     "the cat go from blue house"s,
                                     "the mouse eat blue chees and dance"s };
    const vector<vector<int>> ratings = { { 8, -3 }, { 7, 2, 7 },  { 5, -12, 2, 1 }, { 9 } };
    {
        SearchServer server;
        server.AddDocument(doc_id[0], content[0], DocumentStatus::ACTUAL, ratings[0]);
        server.AddDocument(doc_id[1], content[1], DocumentStatus::ACTUAL, ratings[1]);
        server.AddDocument(doc_id[2], content[2], DocumentStatus::ACTUAL, ratings[2]);
        server.AddDocument(doc_id[3], content[3], DocumentStatus::ACTUAL, ratings[3]);
        ostringstream oss;
        for (const Document& document : server.FindTopDocuments("house from mouse"s)) {
            oss << document.relevance << endl;
        }
        ASSERT_EQUAL(oss.str(), "0.231049\n0.139498\n0.047947\n0.0410974\n");
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocumentAndFindByQuery);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchedWords);
    RUN_TEST(TestSortByRelevanceDocuments);
    RUN_TEST(TestCheckRatingDocuments);
    RUN_TEST(TestUserFilterDocuments);
    RUN_TEST(TestCheckStatusDocuments);
    RUN_TEST(TestCheckRelevanceDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------