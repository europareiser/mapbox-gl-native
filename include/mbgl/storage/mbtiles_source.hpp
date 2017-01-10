#pragma once

#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/offline.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/mapbox.hpp>

#include <unordered_map>
#include <memory>
#include <string>

namespace mapbox {
namespace sqlite {
class Database;
class Statement;
} // namespace sqlite
} // namespace mapbox

namespace mbgl {

class MBTilesSource : private util::noncopyable {
public:

    MBTilesSource(const std::string path);
    ~MBTilesSource();
    
    optional<Response> get(const Resource&);


private:
    void connect(int flags);
    
    class Statement {
    public:
        explicit Statement(mapbox::sqlite::Statement& stmt_) : stmt(stmt_) {}
        Statement(Statement&&) = default;
        Statement(const Statement&) = delete;
        ~Statement();
        
        mapbox::sqlite::Statement* operator->() { return &stmt; };
        
    private:
        mapbox::sqlite::Statement& stmt;
    };
    
    Statement getStatement(const char *);
    
    optional<std::pair<Response, uint64_t>> getInternal(const Resource&);
    optional<std::pair<Response, uint64_t>> getResource(const Resource&);
    optional<std::pair<Response, uint64_t>> getTile(const Resource::TileData&);
    
    const std::string path;
    std::unique_ptr<::mapbox::sqlite::Database> db;
    std::unordered_map<const char *, std::unique_ptr<::mapbox::sqlite::Statement>> statements;
};

} // namespace mbgl
