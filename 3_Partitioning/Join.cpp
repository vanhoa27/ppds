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
#include <unordered_map>
#include <iostream>
#include <gtest/gtest.h>
#include <omp.h>

std::vector<ResultRelation> performJoin(const std::vector<CastRelation>& castRelation, const std::vector<TitleRelation>& titleRelation, int numThreads) {
    omp_set_num_threads(numThreads);
    std::vector<ResultRelation> resultTuples;

    // TODO: Implement your join
    // The benchmark will test it against skewed key distributions
    resultTuples.reserve(castRelation.size() * titleRelation.size() / 2);

    std::unordered_map<uint32_t, std::vector<CastRelation>> hashTable;

    #pragma omp parallel
    {
        std::unordered_map<uint32_t, std::vector<CastRelation>> localHashTable;

        #pragma omp for
        for (size_t i = 0; i < castRelation.size(); ++i) {
            const auto &buildTuple = castRelation[i];
            localHashTable[buildTuple.movieId].push_back(buildTuple);
        }

        #pragma omp critical
        {
            for (const auto &entry : localHashTable) {
                hashTable[entry.first].insert(hashTable[entry.first].end(),
                                               entry.second.begin(), entry.second.end());
            }
        }
    }

    #pragma omp parallel for
    for (size_t i = 0; i < titleRelation.size(); ++i) {
        const auto &probeTuple = titleRelation[i];
        std::vector<ResultRelation> localResults;

        auto it = hashTable.find(probeTuple.titleId);
        if (it != hashTable.end()) {
            for (const auto &buildTuple : it->second) {
                localResults.emplace_back(createResultTuple(buildTuple, probeTuple));
            }
        }

        #pragma omp critical
        {
            resultTuples.insert(resultTuples.end(), localResults.begin(), localResults.end());
        }
    }

    return resultTuples;
}

//==--------------------------------------------------------------------==//
//==------------------------- CORRECTNESS TEST --------------------------==//
//==--------------------------------------------------------------------==//
// TODO: move tests into own directory

std::vector<ResultRelation> testNestedLoopJoin(const std::vector<CastRelation>& leftRelation, const std::vector<TitleRelation>& rightRelation) {
    std::vector<ResultRelation> result;
    for (const auto &l : leftRelation) {
        for (const auto &r : rightRelation) {
            if (l.movieId == r.titleId) {
                result.emplace_back(createResultTuple(l, r));
            }
        }
    }
    return result;
}

TEST(PartioningTest, TestCorrectness) {
    auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"));
    auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"));

    // Makes sure the test data is sorted after their keys
    std::sort(leftRelation.begin(), leftRelation.end(), [](const CastRelation& lhs, const CastRelation& rhs) {
        return lhs.movieId < rhs.movieId;
    });
    std::sort(rightRelation.begin(), rightRelation.end(), [](const TitleRelation& lhs, const TitleRelation& rhs) {
        return lhs.titleId < rhs.titleId;
    });

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

TEST(PartioningTest, TestJoiningTuplesMultipleRuns) {
    constexpr int numRuns = 20;
    Timer timer("Parallelized Join execute");

    std::cout << "Test reading data from a file.\n";
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"), 10000);
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"), 10000);

    timer.start();

    for (int i = 0; i < numRuns; ++i) {
        auto resultTuples = performJoin(leftRelation, rightRelation, 8);
    }

    timer.pause();

    double totalTime = timer.getPrintTime(); // Get overall time in milliseconds
    std::cout << "Total time for " << numRuns << " joins: " << totalTime << " ms" << std::endl;
}