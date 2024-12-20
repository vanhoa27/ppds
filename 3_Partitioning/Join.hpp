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

struct HashFunctionForPartitionExercise {
    std::size_t operator()(const uint64_t& key) const {
        return 42;
    }
};

template <typename T>
using HashMapForPartitionExercise = std::unordered_multimap<uint64_t, T, HashFunctionForPartitionExercise>;

std::vector<ResultRelation> performJoin(const std::vector<CastRelation>& leftRelation, const std::vector<TitleRelation>& rightRelation, int numThreads);

#endif // JOIN_HPP