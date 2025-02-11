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
#include <iostream>
#include <gtest/gtest.h>
#include <vector>
#include <omp.h>
#include <cmath>

inline int findTitleLastHit(const std::vector<TitleRelation> &titleRelation, int targetValue) {
    auto high = std::upper_bound(titleRelation.begin(), titleRelation.end(), targetValue,
                                 [](int value, const TitleRelation &rhs) {
                                     return value < rhs.titleId;
                                 });
    if (high == titleRelation.begin()) return -1;
    --high;
    if (high->titleId == targetValue) {
        return high - titleRelation.begin();
    }
    return -1;
}

inline int findCastLastHit(const std::vector<CastRelation> &castRelation, int targetValue) {
    auto high = std::upper_bound(castRelation.begin(), castRelation.end(), targetValue,
                                 [](int value, const CastRelation &rhs) {
                                     return value < rhs.movieId;
                                 });
    if (high == castRelation.begin()) return -1;
    --high;
    if (high->movieId == targetValue) {
        return high - castRelation.begin();
    }
    return -1;
}

// std::vector<size_t> getPartitionIndices(size_t totalSize, size_t numSections) {
//     std::vector<size_t> partitionIndices;
//     partitionIndices.reserve(numSections);
//
//     double partitionSize = static_cast<double>(totalSize) / (numSections);
//
//     for (int i = 0; i < (numSections - 1); i++) {
//         int end = std::round((i + 1) * partitionSize) - 1;
//         if (end >= totalSize) end = totalSize;
//         partitionIndices.push_back(end);
//     }
//
//     return partitionIndices;
// }


// std::vector<size_t> getPartitionBorders(size_t totalSize, size_t numSections) {
//     std::vector<size_t> partitionIndices;
//     partitionIndices.reserve(numSections + 2);
//     partitionIndices.push_back(0);
//
//     double partitionSize = static_cast<double>(totalSize) / (numSections);
//
//     for (int i = 0; i < (numSections - 1); i++) {
//         int end = std::round((i + 1) * partitionSize) - 1;
//         partitionIndices.push_back(end + 1);
//     }
//
//     partitionIndices.push_back(totalSize + 1);
//
//     return partitionIndices;
// }

// std::vector<size_t> getCastCorrespondingIndices(const std::vector<CastRelation> &castRelation,
//                                                  const std::vector<TitleRelation> &titleRelation,
//                                                  std::vector<size_t> indices, int cutOffSize) {
//     std::vector<size_t> partitionIndices;
//     partitionIndices.reserve(indices.size());
//     partitionIndices.push_back(0);
//
//     for (int i = 1; i < indices.size() - 1; ++i) {
//         partitionIndices.push_back(findCastLastHit(castRelation, titleRelation[indices[i] - 1].titleId) + 1);
//     }
//     partitionIndices.push_back(cutOffSize + 1);
//
//     return partitionIndices;
// }

std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
                                        const std::vector<TitleRelation> &titleRelation, int numThreads) {
    omp_set_num_threads(numThreads);
    // TODO: Implement Sort-Merge Join
    // IMPORTANT: You can assume for this benchmark that the join keys are sorted in both relations.
    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(std::max(castRelation.size(), titleRelation.size()));

    size_t castMaxMovieId = castRelation.back().movieId;
    size_t titleCutOffIndex = findTitleLastHit(titleRelation, castMaxMovieId);
    titleCutOffIndex = (titleCutOffIndex == - 1) ? titleRelation.size() - 1 : titleCutOffIndex;

    size_t titleMaxTitleId = titleRelation.back().titleId;
    size_t castCutOffIndex = findCastLastHit(castRelation, titleMaxTitleId);
    castCutOffIndex = (castCutOffIndex == - 1) ? castRelation.size() - 1 : castCutOffIndex;

    size_t castIndex = 0;
    size_t titleIndex = 0;

    while (castIndex < castCutOffIndex + 1 && titleIndex < titleCutOffIndex + 1) {
        if (castRelation[castIndex].movieId < titleRelation[titleIndex].titleId) {
            ++castIndex;
        } else if (castRelation[castIndex].movieId > titleRelation[titleIndex].titleId) {
            ++titleIndex;
        } else {
            size_t tempTitleIndex = titleIndex;
            while (titleRelation[tempTitleIndex].titleId == castRelation[castIndex].movieId) {
                resultTuples.emplace_back(createResultTuple(castRelation[castIndex], titleRelation[tempTitleIndex]));
                ++tempTitleIndex;
            }
            ++castIndex;
        }
    }

    return resultTuples;
}

// std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
//                                         const std::vector<TitleRelation> &titleRelation, int numThreads) {
//     omp_set_num_threads(numThreads);
//
//     std::vector<ResultRelation> resultTuples;
//     resultTuples.reserve(std::max(castRelation.size(), titleRelation.size()));
//
//     size_t castMaxMovieId = castRelation.back().movieId;
//     size_t titleCutOffIndex = findTitleLastHit(titleRelation, castMaxMovieId);
//     titleCutOffIndex = (titleCutOffIndex == -1) ? titleRelation.size() - 1 : titleCutOffIndex;
//
//     size_t titleMaxTitleId = titleRelation.back().titleId;
//     size_t castCutOffIndex = findCastLastHit(castRelation, titleMaxTitleId);
//     castCutOffIndex = (castCutOffIndex == -1) ? castRelation.size() - 1 : castCutOffIndex;
//
//     size_t castIndex = 0;
//     size_t titleIndex = 0;
//
//     std::vector<size_t> titlePartitionIndices = getPartitionBorders(titleCutOffIndex, 8);
//     std::vector<size_t> castPartitionIndices = getCastCorrespondingIndices(castRelation,titleRelation, titlePartitionIndices, castCutOffIndex);
//
//     std::vector<std::vector<ResultRelation> > threadResults(numThreads);
//
// #pragma omp parallel
//     {
//         int threadId = omp_get_thread_num();
//         std::vector<ResultRelation> &localResults = threadResults[threadId];
//
// #pragma omp for nowait
//         for (size_t i = 0; i < titlePartitionIndices.size() - 1; ++i) {
//             size_t titleStart = titlePartitionIndices[i];
//             size_t titleEnd = titlePartitionIndices[i + 1];
//
//             size_t castStart = castPartitionIndices[i];
//             size_t castEnd = castPartitionIndices[i + 1];
//
//             size_t castIndex = castStart;
//             size_t titleIndex = titleStart;
//
//             while (castIndex < castEnd && titleIndex < titleEnd) {
//                 if (castRelation[castIndex].movieId < titleRelation[titleIndex].titleId) {
//                     ++castIndex;
//                 } else if (castRelation[castIndex].movieId > titleRelation[titleIndex].titleId) {
//                     ++titleIndex;
//                 } else {
//                     size_t tempTitleIndex = titleIndex;
//                     while (tempTitleIndex < titleEnd && titleRelation[tempTitleIndex].titleId == castRelation[castIndex]
//                            .movieId) {
//                         localResults.push_back(
//                             createResultTuple(castRelation[castIndex], titleRelation[tempTitleIndex]));
//                         ++tempTitleIndex;
//                     }
//                     ++castIndex;
//                 }
//             }
//         }
//     }
//
//     for (auto &localResults: threadResults) {
//         resultTuples.insert(resultTuples.end(), localResults.begin(), localResults.end());
//     }
//
//     return resultTuples;
// }


//==--------------------------------------------------------------------==//
//==------------------------- CORRECTNESS TEST --------------------------==//
//==--------------------------------------------------------------------==//
// TODO: move tests into own directory

std::vector<ResultRelation> testNestedLoopJoin(const std::vector<CastRelation> &leftRelation,
                                               const std::vector<TitleRelation> &rightRelation) {
    std::vector<ResultRelation> result;
    for (const auto &l: leftRelation) {
        for (const auto &r: rightRelation) {
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
    std::sort(leftRelation.begin(), leftRelation.end(), [](const CastRelation &lhs, const CastRelation &rhs) {
        return lhs.movieId < rhs.movieId;
    });
    std::sort(rightRelation.begin(), rightRelation.end(), [](const TitleRelation &lhs, const TitleRelation &rhs) {
        return lhs.titleId < rhs.titleId;
    });

    auto resultTuples = performJoin(leftRelation, rightRelation, 8);
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
    constexpr int numRuns = 1;
    Timer timer("Parallelized Join execute");

    std::cout << "Test reading data from a file.\n";
    auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"), 10000);
    auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"), 10000);

    // Makes sure the test data is sorted after their keys
    std::sort(leftRelation.begin(), leftRelation.end(), [](const CastRelation &lhs, const CastRelation &rhs) {
        return lhs.movieId < rhs.movieId;
    });
    std::sort(rightRelation.begin(), rightRelation.end(), [](const TitleRelation &lhs, const TitleRelation &rhs) {
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
