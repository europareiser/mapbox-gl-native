#pragma once

#include <mbgl/storage/file_source.hpp>

namespace mbgl {

namespace util {
template <typename T> class Thread;
} // namespace util

class CascadeFileSource : public FileSource {
public:

    CascadeFileSource(FileSource&);
    ~CascadeFileSource() override;

    bool supportsOptionalRequests() const override {
        return true;
    }

    void setPrimaryFileSourcePath(std::string path);
    std::unique_ptr<AsyncRequest> request(const Resource&, Callback) override;

    class Impl;

private:
    const std::unique_ptr<util::Thread<Impl>> thread;
};

} // namespace mbgl
