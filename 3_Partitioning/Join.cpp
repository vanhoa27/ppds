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
#include "Join.hpp"
#include <omp.h>

// template<typename RelationType>
// std::vector<std::vector<int> > getPartitionIndices(const std::vector<RelationType> &relation, int numThreads) {
//     size_t chunkSize = relation.size() / numThreads;
//
//     std::vector<std::vector<std::vector<int> > > localResults(numThreads, std::vector<std::vector<int> >(8));
//
// #pragma omp parallel num_threads(numThreads)
//     {
//         int threadId = omp_get_thread_num();
//         size_t startIdx = threadId * chunkSize;
//         size_t endIdx = (threadId == numThreads - 1) ? relation.size() : startIdx + chunkSize;
//
//         std::vector<std::vector<int> > &localIndices = localResults[threadId];
//
//         for (size_t i = startIdx; i < endIdx; ++i) {
//             uint32_t id = RelationTraits<RelationType>::getId(relation[i]);
//             const uint8_t lsb = getLSB(id);
//             localIndices[lsb].push_back(i);
//         }
//     }
//
//     std::vector<std::vector<int> > partitionIndices(8);
//
//     for (int i = 0; i < 8; ++i) {
//         size_t totalSize = 0;
//         for (int t = 0; t < numThreads; ++t) {
//             totalSize += localResults[t][i].size();
//         }
//
//         partitionIndices[i].reserve(totalSize);
//         for (int t = 0; t < numThreads; ++t) {
//             partitionIndices[i].insert(partitionIndices[i].end(),
//                                        localResults[t][i].begin(),
//                                        localResults[t][i].end());
//         }
//     }
//
//     return partitionIndices;
// }


// template<typename RelationType>
// std::vector<std::vector<RelationType> > getPartitions(const std::vector<RelationType> &relation, int numThreads) {
//     size_t chunkSize = relation.size() / numThreads;
//
//     std::vector<std::vector<std::vector<RelationType> > > localResults(
//         numThreads, std::vector<std::vector<RelationType> >(8));
//
// #pragma omp parallel num_threads(numThreads)
//     {
//         int threadId = omp_get_thread_num();
//         size_t startIdx = threadId * chunkSize;
//         size_t endIdx = (threadId == numThreads - 1) ? relation.size() : startIdx + chunkSize;
//
//         std::vector<std::vector<RelationType> > &localIndices = localResults[threadId];
//
//         for (size_t i = startIdx; i < endIdx; ++i) {
//             uint32_t id = RelationTraits<RelationType>::getId(relation[i]);
//             const uint8_t lsb = getLSB(id);
//             localIndices[lsb].push_back(relation[i]);
//         }
//     }
//
//     std::vector<std::vector<RelationType> > partitions(8);
//
//     for (int i = 0; i < 8; ++i) {
//         size_t totalSize = 0;
//         for (int t = 0; t < numThreads; ++t) {
//             totalSize += localResults[t][i].size();
//         }
//
//         // partitions[i].reserve(totalSize);
//         for (int t = 0; t < numThreads; ++t) {
//             partitions[i].insert(partitions[i].end(),
//                                  localResults[t][i].begin(),
//                                  localResults[t][i].end());
//         }
//     }
//
//     return partitions;
// }

template<typename RelationType>
std::vector<std::vector<RelationType> > getPartitions(const std::vector<RelationType> &relation, int numThreads) {
    // size_t chunkSize = relation.size() / numThreads;
    // size_t chunkSize = static_cast<size_t>(std::ceil(static_cast<double>(relation.size()) / numThreads));
    std::vector<std::vector<int> > localBuckets(numThreads, std::vector<int>(8, 0));

    // Count occurrences per thread
#pragma omp parallel num_threads(numThreads)
    {
        int threadId = omp_get_thread_num();
        size_t chunkSize = (relation.size() + numThreads - 1) / numThreads;
        size_t startIdx = threadId * chunkSize;
        // size_t endIdx = (threadId == numThreads - 1) ? relation.size() : startIdx + chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, relation.size());

        for (size_t i = startIdx; i < endIdx; ++i) {
            uint32_t id = RelationTraits<RelationType>::getId(relation[i]);
            uint8_t lsb = getLSB(id);
            ++localBuckets[threadId][lsb];
        }
    }

    std::vector<int> globalSizes(numThreads, 0);
    std::vector<std::vector<int> > prefixSum(numThreads, std::vector<int>(8, 0));

#pragma omp parallel for
    for (int i = 0; i < numThreads; ++i) {
        int sum = 0;
        for (int t = 0; t < numThreads; ++t) {
            prefixSum[t][i] = sum;
            sum += localBuckets[t][i];
        }
        globalSizes[i] = sum;
    }


    // Resizing partitions
    std::vector<std::vector<RelationType> > partitions(numThreads);
#pragma omp parallel for
    for (int i = 0; i < 8; ++i) {
        partitions[i].resize(globalSizes[i]);
    }

#pragma omp parallel num_threads(numThreads)
    {
        int threadId = omp_get_thread_num();
        size_t chunkSize = (relation.size() + numThreads - 1) / numThreads;

        size_t startIdx = threadId * chunkSize;
        // size_t endIdx = (threadId == numThreads - 1) ? relation.size() : startIdx + chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, relation.size());

        std::vector<int> localOffsets = prefixSum[threadId];
        for (size_t i = startIdx; i < endIdx; ++i) {
            uint32_t id = RelationTraits<RelationType>::getId(relation[i]);
            uint8_t lsb = getLSB(id);
            partitions[lsb][localOffsets[lsb]++] = relation[i];
        }
    }

    return partitions;
}

std::vector<ResultRelation> nopJoin(const std::vector<TitleRelation> &buildRelation,
                                    const std::vector<CastRelation> &probeRelation) {
    HashMapForPartitionExercise<uint32_t> hashMap;
    std::vector<ResultRelation> resultTuples;
    resultTuples.reserve(std::max(buildRelation.size(), probeRelation.size()));
    // resultTuples.reserve(buildRelation.size() + probeRelation.size());
    hashMap.reserve(buildRelation.size());

    for (size_t i = 0; i < buildRelation.size(); ++i) {
        hashMap.emplace(buildRelation[i].titleId, i);
    }

    for (const auto &probeTuple: probeRelation) {
        __builtin_prefetch(&probeTuple.movieId, 0, 1);
        auto it = hashMap.find(probeTuple.movieId);
        if (it != hashMap.end()) {
            const size_t buildIndex = it->second;
            resultTuples.emplace_back(createResultTuple(probeTuple, buildRelation[buildIndex]));
        }
    }

    return resultTuples;
}

std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &castRelation,
                                        const std::vector<TitleRelation> &titleRelation, int numThreads) {
    // templates verwenden
    // virutal function call nachschauen
    // TODO: Implement your join
    // The benchmark will test it against skewed key distributions
    omp_set_num_threads(numThreads);

    auto buildPartitions = getPartitions(titleRelation, numThreads);
    auto probePartitions = getPartitions(castRelation, numThreads);

    std::vector<std::vector<ResultRelation> > partitionResults(numThreads);

    // #pragma omp parallel for num_threads(numThreads) schedule(guided, 1)
    // for (int i = 0; i < 8; ++i) {
    //     partitionResults[i] = nopJoin(buildPartitions[i], probePartitions[i]);
    // }

#pragma omp parallel
    {
#pragma omp single
        {
            for (int i = 0; i < numThreads; ++i) {
#pragma omp task
                partitionResults[i] = nopJoin(buildPartitions[i], probePartitions[i]);
            }
        }
    }

    std::vector<size_t> offsets(partitionResults.size() + 1, 0);

#pragma omp parallel for
    for (size_t i = 0; i < partitionResults.size(); ++i) {
        offsets[i + 1] = partitionResults[i].size();
    }

    for (size_t i = 1; i < offsets.size(); ++i) {
        offsets[i] += offsets[i - 1];
    }

    std::vector<ResultRelation> resultTuples(offsets.back());

#pragma omp parallel for
    for (size_t i = 0; i < partitionResults.size(); ++i) {
        std::move(partitionResults[i].begin(), partitionResults[i].end(), resultTuples.begin() + offsets[i]);
    }

    return resultTuples;
}

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

TEST(PartioningTest, TestCorrectness) {
    auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"));
    auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"));

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
