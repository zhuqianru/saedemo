#pragma once
#include <iostream>
#include <unordered_map>
#include <string>
#include "io/mgraph.hpp"
#include "serialization/serialization_includes.hpp"
#include "aminer.hpp"
#include "indexing/search.hpp"
#include "indexing/analyzer.hpp"

template <typename T>
inline T parse(const string& data) {
    return sae::serialization::convert_from_string<T>(data);
}

struct AMinerData {
    AMinerData(char const * prefix);

    ~AMinerData() {
    }

    sae::io::MappedGraph* getGraph() const {
        return g.get();
    }

    static AMinerData& instance()
    {
        static AMinerData instance("aminer");
        return instance;
    }

    template<typename T>
    T get(sae::io::vid_t id) const {
        auto vi = g->Vertices();
        vi->MoveTo(id);
        return parse<T>(vi->Data());
    }

    indexing::SearchResult search_publications(const string& query) const;

    std::vector<indexing::Index> pub_index_shards;
    std::unique_ptr<sae::io::MappedGraph> g;
};

