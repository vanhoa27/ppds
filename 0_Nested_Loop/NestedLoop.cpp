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

#include <iostream>
#include <ranges>
#include <gtest/gtest.h>
#include "NestedLoopUtils.hpp"

#include <algorithm>


//==--------------------------------------------------------------------==//
//==------------------------- BENCHMARK TESTS --------------------------==//
//==--------------------------------------------------------------------==//

TEST(NestedLoopTest, TestParsingCastData) {
    std::cout << "Test reading cast data from a file.\n";

    const auto relation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"), 10);

    for(const auto& tuple : relation) {
        std::cout << castRelationToString(tuple) << '\n';
    }
}


TEST(NestedLoopTest, TestParsingTitleData) {
    std::cout << "Test reading data title from a file.\n";

    const auto relation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"), 10);

    for(const auto& tuple : relation) {
        std::cout << titleRelationToString(tuple) << '\n';
    }
}


//==--------------------------------------------------------------------==//
//==------------------- TODO: JOIN IMPLEMENTATION ----------------------==//
//==--------------------------------------------------------------------==//

std::vector<ResultRelation> performNestedLoopJoin(const std::vector<CastRelation>& leftRelation, const std::vector<TitleRelation>& rightRelation) {
    // TODO: Implement the nested loop join algorithm
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

TEST(NestedLoopTest, TestJoiningTuples) {
    std::cout << "Test reading data from a file.\n";

    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"));
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"));

    const auto resultTuples = performNestedLoopJoin(leftRelation, rightRelation);

    std::cout << "\n\n#######################################\n";
    std::cout << "############### RESULTS ###############\n";
    std::cout << "#######################################\n";

    for(const auto& resultTuple : resultTuples) {
        std::cout << resultRelationToString(resultTuple) << '\n';
    }
    std::cout << "\n\n";
}

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

TEST(NestedLoopTest, TestCorrectness) {
    const auto leftRelation = loadCastRelation(DATA_DIRECTORY + std::string("cast_info_uniform.csv"));
    const auto rightRelation = loadTitleRelation(DATA_DIRECTORY + std::string("title_info_uniform.csv"));

    auto resultTuples = performNestedLoopJoin(leftRelation, rightRelation);
    auto testTuples = testNestedLoopJoin(leftRelation, rightRelation);

    // if size mismatch -> direct error
    ASSERT_EQ(resultTuples.size(), testTuples.size()) << "Size mismatch!";

    // sort to ensure comparison
    // std::sort(resultTuples.begin(), resultTuples.end(), [](const ResultRelation &lhs, const ResultRelation &rhs) {
    //     return lhs.movieId < rhs.movieId;
    // });
    // std::sort(testTuples.begin(), testTuples.end(), [](const ResultRelation &lhs, const ResultRelation &rhs) {
    //     return lhs.movieId < rhs.movieId;
    // });
    std::set resultSet(resultTuples.begin(), resultTuples.end());
    std::set testSet(testTuples.begin(), testTuples.end());

    EXPECT_EQ(resultTuples, testTuples) << "The advanced join result does not match the simple join result.";

    std::cout << "Everything is ok.\n";
}

bool operator<(const ResultRelation& lhs, const ResultRelation& rhs) {
    if (lhs.movieId != rhs.movieId)
        return lhs.movieId < rhs.movieId;
    if (lhs.titleId != rhs.titleId)
        return lhs.titleId < rhs.titleId;
    int titleComparison = std::strcmp(lhs.title, rhs.title);
    if (titleComparison != 0)
        return titleComparison < 0;
    if (lhs.productionYear != rhs.productionYear)
        return lhs.productionYear < rhs.productionYear;
    if (lhs.imdbId != rhs.imdbId)
        return lhs.imdbId < rhs.imdbId;
    if (lhs.kindId != rhs.kindId)
        return lhs.kindId < rhs.kindId;
    return lhs.nrOrder < rhs.nrOrder;
}
