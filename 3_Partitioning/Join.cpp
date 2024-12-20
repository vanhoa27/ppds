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

#include <complex>

#include "TimerUtil.hpp"
#include "JoinUtils.hpp"
#include <unordered_map>
#include <iostream>
#include <gtest/gtest.h>
#include <omp.h>

struct LsbHash {
    size_t operator()(uint32_t movieId) const {
        return movieId & 7; // Get the 3 LSB (000 to 111)
    }
};
std::vector<ResultRelation> performJoin(const std::vector<CastRelation>& castRelation, const std::vector<TitleRelation>& titleRelation, int numThreads) {
 // templates verwenden
 // virutal function call nachschauen
    omp_set_num_threads(numThreads);
    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(castRelation.size() * numThreads / 2);

    std::vector<std::vector<CastRelation>> partitions(8);
    for (const auto& cast : castRelation) {
        int partitionId = cast.movieId & 7;
        partitions[partitionId].push_back(cast);
    }

    #pragma omp parallel
    {
        std::vector<ResultRelation> localResult;

        #pragma omp for
        for (int i = 0; i < 8; ++i) {
            std::unordered_map<uint32_t, std::vector<size_t>, LsbHash> hashTable;
            const auto& partition = partitions[i];

            for (size_t idx = 0; idx < partition.size(); ++idx) {
                const auto& cast = partition[idx];
                hashTable[cast.movieId].push_back(idx);
            }

            for (size_t j = 0; j < titleRelation.size(); ++j) {
                const auto& title = titleRelation[j];
                if (auto it = hashTable.find(title.titleId); it != hashTable.end()) {
                    for (const size_t match : it->second) {
                        const auto& cast = partition[match];
                        localResult.push_back(createResultTuple(cast, title));
                    }
                }
            }
        }

        #pragma omp critical
        {
            resultTuples.insert(resultTuples.end(), localResult.begin(), localResult.end());
        }
    }

    return resultTuples;
}

// std::vector<ResultRelation> performJoin(const std::vector<CastRelation>& castRelation, const std::vector<TitleRelation>& titleRelation, int numThreads) {
//     omp_set_num_threads(numThreads);
//     std::vector<ResultRelation> resultTuples;
//     resultTuples.reserve(castRelation.size() * numThreads / 2);
//
//     // TODO: Implement your join
//     // The benchmark will test it against skewed key distributions
//
//     return resultTuples;
// }


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