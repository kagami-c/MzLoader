#include "MzLoader.h"
#include "Loaders.h"

class MzLoader::Impl {
public:
    Impl(const char* filename) {
        std::string filename_str(filename);
        auto suffix_start = filename_str.find_last_of('.');
        auto suffix = filename_str.substr(suffix_start);
        if (suffix == ".mzML") {
            filetype_ = Filetype::mzML;
        }
        else if (suffix == ".mzXML") {
            filetype_ = Filetype::mzXML;
        }
        else {
            throw std::runtime_error("File format is not supported.");
        }
        switch (filetype_) {
        case Filetype::mzML:
            pLoader = std::make_unique<MzmlLoader>(filename);
            break;
        case Filetype::mzXML:
            pLoader = std::make_unique<MzxmlLoader>(filename);
            break;
        }
    }

    bool LoadNext(Spectrum& buffer) const {
        return pLoader->LoadNext(buffer);
    }

private:
    enum class Filetype { mzML, mzXML };
    Filetype filetype_;
    std::unique_ptr<Loader> pLoader;
};

MzLoader::MzLoader(const char* filename) : pImpl(std::make_unique<Impl>(filename)) {}
MzLoader::~MzLoader() {}
bool MzLoader::LoadNext(Spectrum& buffer) { return pImpl->LoadNext(buffer); }
