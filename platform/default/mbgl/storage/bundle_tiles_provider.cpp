#include <mbgl/storage/bundle_tiles_provider.hpp>
#include <mbgl/util/logging.hpp>
#include <boost/format.hpp>

namespace mbgl {

    BundleTilesProvider::BundleTilesProvider(const std::string &path_)
        : path(path_) {
            Log::Info(Event::Database, "Opening sqlite: %s", path.c_str());
            db = std::make_unique<mapbox::sqlite::Database>(path.c_str(), mapbox::sqlite::ReadOnly);
            db->setBusyTimeout(Milliseconds::max());
        
            resourceStmt = std::make_unique<mapbox::sqlite::Statement>(db->prepare("SELECT zoom_level, tile_column, tile_row, tile_data FROM tiles"));
    }
        
    BundleTilesProvider::~BundleTilesProvider() {
        try {
            db.reset();
        } catch (mapbox::sqlite::Exception& ex) {
            Log::Error(Event::Database, ex.code, ex.what());
        }
    }

    bool BundleTilesProvider::nextResource(mbgl::Resource &resource_, mbgl::Response &response_) {
        bool result = false;
        if (resourceStmt->run()) {
            int32_t z = resourceStmt->get<std::int32_t>(0);
            int32_t x = resourceStmt->get<std::int32_t>(1);
            int32_t y = resourceStmt->get<std::int32_t>(2);
            
            boost::format formatter = boost::format{"mapbox://tiles/mapbox.mapbox-terrain-v2,mapbox.mapbox-streets-v7/{%1}/{%2}/{%3}.vector.pbf"} % z % x % y;
            std::string urlTemplate = formatter.str();

            std::shared_ptr<mbgl::Resource> resource = std::make_shared<mbgl::Resource>(Resource::tile(urlTemplate, 1.0f, x, y, z, mbgl::Tileset::Scheme::TMS, mbgl::Resource::Necessity::Required));
            resource_ = *resource;
            
            optional<std::string> data = resourceStmt->get<optional<std::string>>(3);
            std::shared_ptr<mbgl::Response> response = std::make_shared<mbgl::Response>();
            if (!data) {
                response->noContent = true;
            } else {
                response->data = std::make_shared<std::string>(*data);
                response->expires = util::now() + Seconds(60 * 60 * 24 * 5);//5 days 
            }
            
            response_ = *response;
            result = true;
        }
        return result;
    }
        
    void BundleTilesProvider::reset() {
        resourceStmt.reset();
    }

}
