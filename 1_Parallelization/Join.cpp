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
#include <atomic>
#include <numeric>
#include <tbb/concurrent_hash_map.h>

// std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
//                                         const std::vector<TitleRelation> &titleRelation, int numThreads) {
//     omp_set_num_threads(numThreads);
//     std::vector<ResultRelation> resultTuples;
//     resultTuples.reserve(std::min(castRelation.size(), titleRelation.size()));
//
//     const std::vector<CastRelation> &buildRelation = castRelation;
//     const std::vector<TitleRelation> &probeRelation = titleRelation;
//
//     std::unordered_map<uint32_t, std::vector<CastRelation>> hashTable;
//     hashTable.reserve(buildRelation.size());
//
//     for (size_t i = 0; i < buildRelation.size(); ++i) {
//         const auto &castTuple = buildRelation[i];
//         hashTable[castTuple.movieId].push_back(castTuple);
//     }
//
//     for (size_t i = 0; i < probeRelation.size(); ++i) {
//         const auto &titleTuple = probeRelation[i];
//         uint32_t titleId = titleTuple.titleId;
//
//         auto it = hashTable.find(titleId);
//         if (it != hashTable.end()) {
//             for (const auto &castTuple: it->second) {
//                 resultTuples.emplace_back(createResultTuple(castTuple, titleTuple));
//             }
//         }
//     }
//
//     return resultTuples;
// }


std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
                                        const std::vector<TitleRelation> &titleRelation, int numThreads) {
    omp_set_num_threads(numThreads);
    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(std::min(castRelation.size(), titleRelation.size()));

    const std::vector<CastRelation> &buildRelation = castRelation;
    const std::vector<TitleRelation> &probeRelation = titleRelation;

    tbb::concurrent_hash_map<uint32_t, std::vector<CastRelation>> hashTable;

    #pragma omp parallel for
    for (size_t i = 0; i < buildRelation.size(); ++i) {
        const auto &castTuple = buildRelation[i];
        tbb::concurrent_hash_map<uint32_t, std::vector<CastRelation>>::accessor acc;
        hashTable.insert(acc, castTuple.movieId);
        acc->second.push_back(castTuple);
    }

    for (size_t i = 0; i < probeRelation.size(); ++i) {
        const auto &titleTuple = probeRelation[i];
        uint32_t titleId = titleTuple.titleId;

        tbb::concurrent_hash_map<uint32_t, std::vector<CastRelation>>::const_accessor acc;
        if (hashTable.find(acc, titleId)) {
            for (const auto &castTuple : acc->second) {
                resultTuples.emplace_back(createResultTuple(castTuple, titleTuple));
            }
        }
    }

    return resultTuples;
}



TEST(ParallelizationTest, TestJoiningTuples) {
    std::cout << "Test reading data from a file.\n";
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"), 10000);
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"), 10000);

    Timer timer("Parallelized Join execute");
    timer.start();

    auto resultTuples = performJoin(leftRelation, rightRelation, 8);

    timer.pause();

    std::cout << "Timer: " << timer << std::endl;
    std::cout << "Result size: " << resultTuples.size() << std::endl;
    std::cout << "\n\n";
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

TEST(ParallelizationTest, TestCorrectness) {
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"));
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"));

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

TEST(ParallelizationTest, TestJoiningTuplesMultipleRuns) {
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