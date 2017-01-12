#include <mbgl/storage/mbtiles_source.hpp>
#include <mbgl/platform/log.hpp>

//#include <mbgl/util/platform.hpp>
#include <mbgl/util/url.hpp>
#include <mbgl/util/thread.hpp>
#include <mbgl/util/compression.hpp>

#include <cassert>
#include <unordered_map>

#include "sqlite3.hpp"
#include <sqlite3.h>

namespace mbgl {

MBTilesSource::Statement::~Statement() {
    stmt.reset();
    stmt.clearBindings();
}

MBTilesSource::MBTilesSource(std::string path_)
    : path(std::move(path_)) {
        connect(mapbox::sqlite::ReadOnly);
}

MBTilesSource::~MBTilesSource() {
    // Deleting these SQLite objects may result in exceptions, but we're in a destructor, so we
    // can't throw anything.
    try {
        statements.clear();
        db.reset();
    } catch (mapbox::sqlite::Exception& ex) {
        Log::Error(Event::Database, ex.code, ex.what());
    }
}
    
optional<Response> MBTilesSource::get(const Resource& resource) {
    auto result = getInternal(resource);
    if (result) {
        return result->first;
    } else {
        optional<Response> offlineResponse = optional<Response>();
        offlineResponse.emplace();
        offlineResponse->noContent = true;
        offlineResponse->error = std::make_unique<Response::Error>(Response::Error::Reason::NotFound, "Not found in offline database");
        return offlineResponse;
    }
}

#pragma mark Private
void MBTilesSource::connect(int flags) {
    db = std::make_unique<mapbox::sqlite::Database>(path.c_str(), flags);
    db->setBusyTimeout(Milliseconds::max());
}
    
MBTilesSource::Statement MBTilesSource::getStatement(const char * sql) {
    auto it = statements.find(sql);
    
    if (it != statements.end()) {
        return Statement(*it->second);
    }
    
    return Statement(*statements.emplace(sql, std::make_unique<mapbox::sqlite::Statement>(db->prepare(sql))).first->second);
}
    
optional<std::pair<Response, uint64_t>> MBTilesSource::getInternal(const Resource& resource) {
    if (resource.kind == Resource::Kind::Tile) {
        assert(resource.tileData);
        return getTile(*resource.tileData);
    } else {
        return getResource(resource);
    }
}
    
optional<std::pair<Response, uint64_t>> MBTilesSource::getTile(const Resource::TileData& tile) {
    Statement stmt = getStatement("SELECT tile_data FROM tiles WHERE zoom_level = ?1 AND tile_column = ?2 AND tile_row = ?3");
    int32_t y = (1 << tile.z) - tile.y - 1; // flip y for mbtiles
    Log::Info(Event::Database, "Finding tile (%d, %d, %d [%d])", tile.z, tile.x, tile.y, y);
    
    stmt->bind(1, tile.z);
    stmt->bind(2, tile.x);
    stmt->bind(3, y);
    
    if (!stmt->run()) {
        return {};
    }
    
    Response response;
    uint64_t size = 0;
    
    optional<std::string> data = stmt->get<optional<std::string>>(0);
    if (!data) {
        response.noContent = true;
    } else {
        response.data = std::make_shared<std::string>(data);
        size = data->length();
        Log::Info(Event::Database, "==> tile (%d, %d, %d [%d])", tile.z, tile.x, tile.y, y);
    }
    return std::make_pair(response, size);
}
    
optional<std::pair<Response, uint64_t>> MBTilesSource::getResource(const Resource& resource) {
    // clang-format off
    Statement stmt = getStatement(
                                  //        0      1        2       3        4
                                  "SELECT etag, expires, modified, data, compressed "
                                  "FROM resources "
                                  "WHERE url = ?");
    // clang-format on
    
    stmt->bind(1, resource.url);
    
    if (!stmt->run()) {
        return {};
    }
    
    Response response;
    uint64_t size = 0;
    
    response.etag = stmt->get<optional<std::string>>(0);
    response.expires = stmt->get<optional<Timestamp>>(1);
    response.modified = stmt->get<optional<Timestamp>>(2);
    
    optional<std::string> data = stmt->get<optional<std::string>>(3);
    if (!data) {
        response.noContent = true;
    } else if (stmt->get<int>(4)) {
        response.data = std::make_shared<std::string>(util::decompress(*data));
        size = data->length();
    } else {
        response.data = std::make_shared<std::string>(*data);
        size = data->length();
    }
    
    return std::make_pair(response, size);
}

} // namespace mbgl
