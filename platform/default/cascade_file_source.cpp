#include <mbgl/storage/cascade_file_source.hpp>

#include <mbgl/storage/mbtiles_source.hpp>
#include <mbgl/util/url.hpp>
#include <mbgl/util/thread.hpp>
#include <mbgl/util/work_request.hpp>

#include <cassert>
#include <unordered_map>

namespace mbgl {

class CascadeFileSource::Impl {
public:
    Impl(FileSource& defaultFileSource_)
        : defaultFileSource(&defaultFileSource_), primaryFileSource(nullptr){
    }

    void request(AsyncRequest* req, Resource resource, Callback callback) {
        if (primaryFileSource) {
            optional<Response> offlineResponse = primaryFileSource->get(resource);
            if (offlineResponse->noContent) {
                defaultTasks[req] = defaultFileSource->request(resource, callback);
            } else {
                callback(*offlineResponse);
            }
        } else {
            defaultTasks[req] = defaultFileSource->request(resource, callback);
        }
    }

    void cancel(AsyncRequest* req) {
        defaultTasks.erase(req);
    }

    void setPrimaryFileSourcePath(std::string path_) {
        primaryFileSource = std::make_unique<MBTilesSource>(path_);
    }

private:
    const std::unique_ptr<FileSource> defaultFileSource;
    std::unique_ptr<MBTilesSource> primaryFileSource;
    
    std::unordered_map<AsyncRequest*, std::unique_ptr<AsyncRequest>> defaultTasks;
};

CascadeFileSource::CascadeFileSource(FileSource& defaultFileSource)
    : thread(std::make_unique<util::Thread<Impl>>(util::ThreadContext{"CascadeFileSource", util::ThreadPriority::Low}, defaultFileSource)) {
}

CascadeFileSource::~CascadeFileSource() = default;

std::unique_ptr<AsyncRequest> CascadeFileSource::request(const Resource& resource, Callback callback) {
    class CascadeFileRequest : public AsyncRequest {
    public:
        CascadeFileRequest(Resource resource_, FileSource::Callback callback_, util::Thread<CascadeFileSource::Impl>& thread_)
            : thread(thread_),
              workRequest(thread.invokeWithCallback(&CascadeFileSource::Impl::request, this, resource_, callback_)) {
        }

        ~CascadeFileRequest() override {
            thread.invoke(&CascadeFileSource::Impl::cancel, this);
        }

        util::Thread<CascadeFileSource::Impl>& thread;
        std::unique_ptr<AsyncRequest> workRequest;
    };
    return std::make_unique<CascadeFileRequest>(resource, callback, *thread);
}

void CascadeFileSource::setPrimaryFileSourcePath(std::string path) {
    thread->invoke(&Impl::setPrimaryFileSourcePath, std::move(path));
}

} // namespace mbgl

