#pragma once

#include <mbgl/storage/resource.hpp>
#include "sqlite3.hpp"
#include <sqlite3.h>

namespace mbgl {

class BundleTilesProvider {
    public:
        uint64_t getOfflineMapboxTileCount();
        bool nextResource(mbgl::Resource &resource, mbgl::Response &response);
        void reset();
        
        explicit BundleTilesProvider(const std::string &path);
        BundleTilesProvider(BundleTilesProvider&&) = default;
        BundleTilesProvider(const BundleTilesProvider&) = delete;
        ~BundleTilesProvider();

    private:
        std::string path;
        std::unique_ptr<::mapbox::sqlite::Database> db;
        std::unique_ptr<::mapbox::sqlite::Statement> resourceStmt;
    };
    
} // namespace mbgl
