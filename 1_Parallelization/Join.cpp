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

// unordered multimap
std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
                                        const std::vector<TitleRelation> &titleRelation, int numThreads) {
    omp_set_num_threads(numThreads);

    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(std::max(castRelation.size(), titleRelation.size()));

    const std::vector<CastRelation> &probeRelation = castRelation;
    const std::vector<TitleRelation> &buildRelation = titleRelation;

    std::unordered_multimap<uint32_t, size_t> hashTable;
    hashTable.reserve(buildRelation.size() * 1.5);

    for (size_t i = 0; i < buildRelation.size(); ++i) {
        const auto& buildTuple = buildRelation[i];
        hashTable.emplace(buildTuple.titleId, i);
    }

    for (const auto &probeTuple : probeRelation) {
        auto [begin, end] = hashTable.equal_range(probeTuple.movieId);
        for (auto it = begin; it != end; ++it) {
            const size_t buildIndex = it->second;
            const auto &buildTuple = buildRelation[buildIndex];
            resultTuples.emplace_back(createResultTuple(probeTuple, buildTuple));
        }
    }

    // std::vector<ResultRelation> resultTuples;
    // resultTuples.reserve(castRelation.size());
    //
    // for (const auto &cast : castRelation) {
    //     int probeIndex = cast.movieId - 1;
    //
    //     if (probeIndex >= 0 && probeIndex < titleRelation.size()) {
    //         const auto &title = titleRelation[probeIndex];
    //         resultTuples.emplace_back(createResultTuple(cast, title));
    //     }
    // }

    return resultTuples;
}

// std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
//                                         const std::vector<TitleRelation> &titleRelation, int numThreads) {
//     omp_set_num_threads(numThreads);
//
//     std::vector<ResultRelation> resultTuples;
//     if (castRelation.size() > 10000) {
//         resultTuples.reserve(std::min(castRelation.size(), titleRelation.size()));
//         int probeIndex;
//         for (const auto &cast : castRelation) {
//             probeIndex = cast.movieId - 1;
//
//             if (probeIndex >= 0 && probeIndex < titleRelation.size()) {
//                 const auto &title = titleRelation[probeIndex];
//                 resultTuples.emplace_back(createResultTuple(cast, title));
//             }
//         }
//     }
//
//     return resultTuples;
// }


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

//==--------------------------------------------------------------------==//S
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
