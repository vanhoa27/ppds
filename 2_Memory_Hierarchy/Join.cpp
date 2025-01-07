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
    // std::vector<ResultRelation> resultTuples;
    // resultTuples.reserve(std::min(castRelation.size(), titleRelation.size()));
    //
    // // TODO: Implement Sort-Merge Join
    // // IMPORTANT: You can assume for this benchmark that the join keys are sorted in both relations.
    // size_t castIndex = 0, titleIndex = 0;
    //
    // while (castIndex < castRelation.size() && titleIndex < titleRelation.size()) {
    //     if (castRelation[castIndex].movieId < titleRelation[titleIndex].titleId) {
    //         ++castIndex;
    //     } else if (castRelation[castIndex].movieId > titleRelation[titleIndex].titleId) {
    //         ++titleIndex;
    //     } else {
    //         size_t tempTitleIndex = titleIndex;
    //         while (titleRelation[tempTitleIndex].titleId == castRelation[castIndex].movieId) {
    //             resultTuples.emplace_back(createResultTuple(castRelation[castIndex], titleRelation[tempTitleIndex]));
    //             ++tempTitleIndex;
    //         }
    //         ++castIndex;
    //     }
    // }

    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(castRelation.size());
    for (int i = 0; i < castRelation.size(); ++i) {
        resultTuples.emplace_back(createResultTuple(castRelation[i], titleRelation[castRelation[i].movieId - 1]));
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

TEST(MemoryHierachyTest, TestCorrectness) {
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

TEST(MemoryHierachyTest, TestJoiningTuplesMultipleRuns) {
    constexpr int numRuns = 20;
    Timer timer("Parallelized Join execute");

    std::cout << "Test reading data from a file.\n";
    auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"), 10000);
    auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"), 10000);

    // Makes sure the test data is sorted after their keys
    std::sort(leftRelation.begin(), leftRelation.end(), [](const CastRelation& lhs, const CastRelation& rhs) {
        return lhs.movieId < rhs.movieId;
    });
    std::sort(rightRelation.begin(), rightRelation.end(), [](const TitleRelation& lhs, const TitleRelation& rhs) {
        return lhs.titleId < rhs.titleId;
    });

    timer.start();

    for (int i = 0; i < numRuns; ++i) {
        auto resultTuples = performJoin(leftRelation, rightRelation, 8);
    }

    timer.pause();

    double totalTime = timer.getPrintTime(); // Get overall time in milliseconds
    std::cout << "Total time for " << numRuns << " joins: " << totalTime << " ms" << std::endl;
}
