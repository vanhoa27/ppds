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
#ifndef JOIN_HPP
#define JOIN_HPP

#include "JoinUtils.hpp"
#include <unordered_map>
#include <cstdint>
#include <omp.h>
#include <numeric>

inline uint8_t getLSB(uint32_t movieId) {
    std::hash<uint32_t> hasher;
    return static_cast<uint8_t>(hasher(movieId) & 7);
}

template<typename RelationType>
struct RelationTraits;

template<>
struct RelationTraits<TitleRelation> {
    static uint32_t getId(const TitleRelation &relation) {
        return relation.titleId;
    }
};

template<>
struct RelationTraits<CastRelation> {
    static uint32_t getId(const CastRelation &relation) {
        return relation.movieId;
    }
};

class RadixPartitioner {
public:
    std::vector<size_t> buildHistogram;
    std::vector<size_t> probeHistogram;

    std::vector<size_t> buildPrefixSumTable;
    std::vector<size_t> probePrefixSumTable;
    int numThreads;

    RadixPartitioner(int threadNums)
        : numThreads(threadNums),
          buildHistogram(threadNums, 0), probeHistogram(threadNums, 0), buildPrefixSumTable(threadNums + 1, 0),
          probePrefixSumTable(threadNums + 1, 0) {
    }

    void createHistograms(const std::vector<CastRelation> &castRelation,
                          const std::vector<TitleRelation> &titleRelation) {
        computeHistogram(castRelation, buildHistogram);
        computeHistogram(titleRelation, probeHistogram);
    }

    void createPrefixSumTables() {
        computePrefixSumTable(buildPrefixSumTable, buildHistogram);
        computePrefixSumTable(probePrefixSumTable, probeHistogram);
    }

private:
    template<typename RelationType>
    void computeHistogram(const std::vector<RelationType> &relation, std::vector<size_t> &histogram) {
        size_t chunkSize = relation.size() / numThreads;
        std::vector localResults(numThreads, std::vector<int>(8, 0));

#pragma omp parallel num_threads(numThreads)
        {
            int threadId = omp_get_thread_num();
            size_t startIdx = threadId * chunkSize;
            size_t endIdx = (threadId == numThreads - 1) ? relation.size() : startIdx + chunkSize;

            std::vector<int> &localIndices = localResults[threadId];

            for (size_t i = startIdx; i < endIdx; ++i) {
                uint32_t id = RelationTraits<RelationType>::getId(relation[i]);
                const uint8_t lsb = getLSB(id);
                ++localIndices[lsb];
            }
        }

        for (size_t i = 0; i < 8; ++i) {
            for (size_t t = 0; t < numThreads; ++t) {
                histogram[i] += localResults[t][i];
            }
        }
    }

    void computePrefixSumTable(
        std::vector<size_t> &prefixSumTable, const std::vector<size_t> &histogram) {
        prefixSumTable[0] = 0;
        size_t offset = -1;
        for (size_t i = 1; i < histogram.size() + 1; ++i) {
            offset += histogram[i - 1];
            prefixSumTable[i] = offset;
        }
    }

    template<typename RelationType>
    void placeRecord(const std::vector<size_t> &histogram, const std::vector<size_t> &prefixSumTable,
                     const std::vector<RelationType> &relation) {
    }
};

struct HashFunctionForPartitionExercise {
    std::size_t operator()(const uint64_t &key) const {
        std::hash<uint64_t> hasher;
        return hasher(key);
        //        return 42;
    }
};

template<typename T>
using HashMapForPartitionExercise = std::unordered_multimap<uint64_t, T, HashFunctionForPartitionExercise>;

std::vector<ResultRelation> performJoin(const std::vector<CastRelation> &leftRelation,
                                        const std::vector<TitleRelation> &rightRelation, int numThreads);

#endif // JOIN_HPP
