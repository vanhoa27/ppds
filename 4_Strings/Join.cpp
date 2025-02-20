/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "TimerUtil.hpp"
#include "JoinUtils.hpp"
#include "Util/include/Trie.hpp"
#include <iostream>
#include <gtest/gtest.h>
#include <omp.h>

std::vector<ResultRelation> performJoin(const std::vector<CastRelation>& castRelation, const std::vector<TitleRelation>& titleRelation, int numThreads) {
    omp_set_num_threads(numThreads);
    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(std::max(castRelation.size(), titleRelation.size()));

    // TODO: Implement a join on the strings cast.note and title.title
    // The benchmark will join on increasing string sizes: cast.note% LIKE title.title
    Trie trie;

    for (size_t i = 0; i < titleRelation.size(); ++i) {
        trie.insertKey(titleRelation[i].title, i);
    }

    std::vector<std::vector<ResultRelation>> threadResults(numThreads);

    #pragma omp parallel
    {
        int threadID = omp_get_thread_num();
        std::vector<ResultRelation>& localResults = threadResults[threadID];

        #pragma omp for schedule(dynamic, numThreads)
        for (size_t i = 0; i < castRelation.size(); ++i) {
            auto indices = trie.searchPrefix(castRelation[i].note);
            for (const auto& index : indices) {
                localResults.emplace_back(createResultTuple(castRelation[i], titleRelation[index]));
            }
        }
    }

    for (const auto& localResults : threadResults) {
        resultTuples.insert(resultTuples.end(), localResults.begin(), localResults.end());
    }

    return resultTuples;
}

//==--------------------------------------------------------------------==//
//==------------------------- CORRECTNESS TEST --------------------------==//
//==--------------------------------------------------------------------==//
// TODO: move tests into own directory

bool startsWith(const std::string& fullString, const std::string& prefix) {
    return fullString.rfind(prefix, 0) == 0;  // Checks if fullString starts with prefix
}

std::vector<ResultRelation> testNestedLoopJoin(const std::vector<CastRelation>& leftRelation, const std::vector<TitleRelation>& rightRelation) {
    std::vector<ResultRelation> result;
    for (const auto &l : leftRelation) {
        for (const auto &r : rightRelation) {
            if (startsWith(r.title, l.note)) {  // Match when note starts with title
                result.emplace_back(createResultTuple(l, r));
            }
        }
    }
    return result;
}

TEST(StringTest, TestCorrectness) {
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast.csv"), 2000);
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title.csv"), 2000);

    auto resultTuples = performJoin(leftRelation, rightRelation,8);
    auto testTuples = testNestedLoopJoin(leftRelation, rightRelation);

    // if size mismatch -> direct error
    ASSERT_EQ(resultTuples.size(), testTuples.size()) << "Size mismatch!";

    // sort to ensure comparison
    std::sort(resultTuples.begin(), resultTuples.end(), [](const ResultRelation &lhs, const ResultRelation &rhs) {
        return std::tie(lhs.movieId, lhs.castInfoId, lhs.titleId) < std::tie(rhs.movieId, rhs.castInfoId, rhs.titleId);
    });
    std::sort(testTuples.begin(), testTuples.end(), [](const ResultRelation &lhs, const ResultRelation &rhs) {
        return std::tie(lhs.movieId, lhs.castInfoId, lhs.titleId) < std::tie(rhs.movieId, rhs.castInfoId, rhs.titleId);
    });

    EXPECT_EQ(resultTuples, testTuples) << "The advanced join result does not match the simple join result.";

    std::cout << "Everything is ok.\n";
}


//==--------------------------------------------------------------------==//
//==------------------------- BENCHMARK TESTS --------------------------==//
//==--------------------------------------------------------------------==//

TEST(StringTest, TestJoiningTuplesMultipleRuns) {
    constexpr int numRuns = 20;
    Timer timer("Parallelized Join execute");

    std::cout << "Test reading data from a file.\n";
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast.csv"), 2000);
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title.csv"), 2000);

    timer.start();

    for (int i = 0; i < numRuns; ++i) {
        auto resultTuples = performJoin(leftRelation, rightRelation, 8);
    }

    timer.pause();

    double totalTime = timer.getPrintTime(); // Get overall time in milliseconds
    std::cout << "Total time for " << numRuns << " joins: " << totalTime << " ms" << std::endl;
}

