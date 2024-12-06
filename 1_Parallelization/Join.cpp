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

    // TODO: Improve on the nested loop join
    for (const auto& castTuple : castRelation) {
        for (const auto& titleTuple : titleRelation) {
            if (castTuple.movieId == titleTuple.titleId) {
                resultTuples.push_back(createResultTuple(castTuple, titleTuple));
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
