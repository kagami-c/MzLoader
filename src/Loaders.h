#pragma once

#include "MzLoader.h"
#include "Decode.h"
#include <queue>
#include <cstring>

// solve naming conflict between rapidxml and zlib by adding a macro
#define alloc_func rapidxml_alloc_func
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#undef alloc_func

using std::vector;
using std::pair;

class Loader {
public:
    Loader(const char* filename) : filename_(filename) {}
    virtual ~Loader() {};
    virtual std::string ToString() const = 0;
    virtual bool LoadNext(MzLoader::Spectrum& buffer) = 0;

protected:
    const char* filename_;

    // helper functions
    static bool NodeNameIs(rapidxml::xml_node<>* node, const char* reference) {
        return (0 == strncmp(node->name(), reference, node->name_size()));
    }

    static bool AttrValueIs(rapidxml::xml_attribute<>* attr, const char* reference) {
        return (0 == strncmp(attr->value(), reference, attr->value_size()));
    }

    static std::string GetAttrValue(rapidxml::xml_attribute<>* attr) {
        return std::string(attr->value(), attr->value_size());
    }
};

class MzmlLoader : public Loader {
public:
    MzmlLoader(const char* filename) : Loader(filename), file_(filename) {
        doc_.parse<0>(file_.data());
        auto mzml_root = doc_.first_node("indexedmzML") != nullptr
                         ? doc_.first_node("indexedmzML")->first_node("mzML")
                         : doc_.first_node("mzML");
        next_spectrum_node_ = mzml_root->first_node("run")->first_node("spectrumList")->first_node("spectrum");
    }
    ~MzmlLoader() override {}
    std::string ToString() const override { return "<Loader format=mzML path=" + std::string(filename_) + '>'; }

    bool LoadNext(MzLoader::Spectrum& buffer) override {
        while (next_spectrum_node_ != nullptr) {
            auto are_params_complete = SetParams(buffer, next_spectrum_node_);
            if (!are_params_complete) { next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue; }
            if (buffer.ms_level != 2) { next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue; }

            auto precursor_info_exist = SetPrecursorInfo(buffer, next_spectrum_node_);
            if (!precursor_info_exist) { next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue; }
            auto precursor_molecule_weight = buffer.precursor_mz * buffer.precursor_charge - buffer.precursor_charge * 1.007;
//            if (precursor_molecule_weight < 700 || 5000 < precursor_molecule_weight) {
//                next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue;
//            }

            auto scan_num_exist = SetScanNum(buffer, next_spectrum_node_);
            if (!scan_num_exist) { next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue; }

            auto mz_int_exist = SetMzIntensity(buffer, next_spectrum_node_);
            if (!mz_int_exist) { next_spectrum_node_ = next_spectrum_node_->next_sibling(); continue; }

            // pass all checks, next_spectrum_node_ remains the same now.
            break;
        }
        if (next_spectrum_node_ != nullptr) {
            next_spectrum_node_ = next_spectrum_node_->next_sibling();
            return true;
        }
        else {
            return false;
        }
    }

private:
    rapidxml::file<> file_;
    rapidxml::xml_document<> doc_;
    rapidxml::xml_node<>* next_spectrum_node_;

    // builders
    static bool SetScanNum(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* spectrum_node) {
        std::string id = spectrum_node->first_attribute("id")->value();
        auto scan_start = id.find("scan=");
        buffer.scan_num = stoi(id.substr(scan_start + 5));
        return true;
    }

    static bool SetParams(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* spectrum_node) {
        bool set_ms_level = false;
        bool set_base_peak_mz = false;
        bool set_base_peak_intensity = false;
        bool set_total_ion_current = false;
        for (auto param_node = spectrum_node->first_node("cvParam");
                param_node && NodeNameIs(param_node, "cvParam");
                param_node = param_node->next_sibling()) {
            auto name_attr = param_node->first_attribute("name");
            auto field_value = GetAttrValue(param_node->first_attribute("value"));
            if (AttrValueIs(name_attr, "ms level")) {
                buffer.ms_level = stoi(field_value);
                set_ms_level = true;
            }
            if (AttrValueIs(name_attr, "base peak m/z")) {
                buffer.base_peak_mz = stod(field_value);
                set_base_peak_mz = true;
            }
            if (AttrValueIs(name_attr, "base peak intensity")) {
                buffer.base_peak_intensity = stod(field_value);
                set_base_peak_intensity = true;
            }
            if (AttrValueIs(name_attr, "total ion current")) {
                buffer.total_ion_current = stod(field_value);
                set_total_ion_current = true;
            }
        }
        return set_ms_level && set_base_peak_mz && set_base_peak_intensity && set_total_ion_current;
    }

    static bool SetPrecursorInfo(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* spectrum_node) {
        if (spectrum_node->first_node("precursorList") == nullptr) { return false; }  // MS2 spectrum should has precursor info.
        bool set_charge = false;
        bool set_mz = false;
        auto precursor_node = spectrum_node->first_node("precursorList")->first_node("precursor")
                ->first_node("selectedIonList")->first_node("selectedIon");
        for (auto param_node = precursor_node->first_node("cvParam");
                param_node && NodeNameIs(param_node, "cvParam");
                param_node = param_node->next_sibling()) {
            auto name_attr = param_node->first_attribute("name");
            auto field_value = GetAttrValue(param_node->first_attribute("value"));
            if (AttrValueIs(name_attr, "charge state")) {
                buffer.precursor_charge = stoi(field_value);
                set_charge = true;
            }
            if (AttrValueIs(name_attr, "selected ion m/z")) {
                buffer.precursor_mz = stod(field_value);
                set_mz = true;
            }
        }
        return set_charge && set_mz;
    }

    static bool SetMzIntensity(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* spectrum_node) {
        bool set_mz_list = false;
        bool set_intensity_list = false;
        vector<double> mz_list;
        vector<double> intensity_list;
        for (auto binary_data_array_node = spectrum_node->first_node("binaryDataArrayList")->first_node("binaryDataArray");
                binary_data_array_node && NodeNameIs(binary_data_array_node, "binaryDataArray");
                binary_data_array_node = binary_data_array_node->next_sibling()) {
            // extract params for decoding
            bool set_precision = false;
            int precision = 64;
            bool set_compress = false;
            bool is_compressed = false;
            // determine which array it is
            bool is_mz = false;
            bool is_int = false;
            for (auto param_node = binary_data_array_node->first_node("cvParam");
                    param_node && NodeNameIs(param_node, "cvParam");
                    param_node = param_node->next_sibling()) {
                auto name_attr = param_node->first_attribute("name");
                if (AttrValueIs(name_attr, "64-bit float")) {
                    if (set_precision) { return false; }  // already set precision, data error
                    precision = 64;
                    set_precision = true;
                }
                if (AttrValueIs(name_attr, "32-bit float")) {
                    if (set_precision) { return false; }  // already set precision, data error
                    precision = 32;
                    set_precision = true;
                }
                if (AttrValueIs(name_attr, "no compression")) {
                    if (set_compress) { return false; }  // already set compress state, data error
                    is_compressed = false;
                    set_compress = true;
                }
                if (AttrValueIs(name_attr, "zlib compression")) {
                    if (set_compress) { return false; }  // already set compress state, data error
                    is_compressed = true;
                    set_compress = true;
                }
                if (AttrValueIs(name_attr, "m/z array")) {
                    if (is_mz || is_int) { return false; }  // already choose one type, data error
                    is_mz = true;
                }
                if (AttrValueIs(name_attr, "intensity array")) {
                    if (is_mz || is_int) { return false; }  // already choose one type, data error
                    is_int = true;
                }
            }
            // check all parameters are set
            auto mz_int_valid = (is_mz || is_int) && !(is_mz && is_int);
            if (!set_precision || !set_compress || !mz_int_valid) { return false; }  // parameters are not enough, data error.
            // decode
            auto raw_data = binary_data_array_node->first_node("binary")->value();
            auto raw_data_size = binary_data_array_node->first_node("binary")->value_size();
            auto decoded_data = DecodeMzData(raw_data, raw_data_size, precision, is_compressed, true);  // little endian
            if (is_mz) { mz_list = decoded_data; set_mz_list = true; }
            if (is_int) { intensity_list = decoded_data; set_intensity_list = true; }
        }
        if (set_mz_list && set_intensity_list) {
            if (mz_list.size() != intensity_list.size()) { return false; }  // data error, should be the same size
            else {  // build peaks and return
                vector<pair<double, double>> peaks(mz_list.size());
                for (auto i = 0; i < mz_list.size(); ++i) {
                    peaks[i] = std::make_pair(mz_list[i], intensity_list[i]);
                }
                buffer.peaks = peaks;
                return true;
            }
        }
        else {  // one of mz or int has not been set
            return false;
        }
    }
};

class MzxmlLoader : public Loader {
public:
    MzxmlLoader(const char* filename) : Loader(filename), file_(filename) {
        doc_.parse<0>(file_.data());
        for (auto scan_node = doc_.first_node("mzXML")->first_node("msRun")->first_node("scan");
                scan_node && NodeNameIs(scan_node, "scan"); scan_node = scan_node->next_sibling()) {
            untreated_scan_nodes_.push(scan_node);
        }
    }
    ~MzxmlLoader() override {}
    std::string ToString() const override { return "<Loader format=mzXML path=" + std::string(filename_) + '>'; }

    bool LoadNext(MzLoader::Spectrum& buffer) override {
        auto current_scan_node = GetNextScan();
        while (current_scan_node != nullptr) {
            auto are_params_complete = SetParams(buffer, current_scan_node);
            if (!are_params_complete) { current_scan_node = GetNextScan(); continue; }
            if (buffer.ms_level != 2) { current_scan_node = GetNextScan(); continue; }

            auto precursor_info_exist = SetPrecursorInfo(buffer, current_scan_node);
            if (!precursor_info_exist) { current_scan_node = GetNextScan(); continue; }
            auto precursor_molecule_weight = buffer.precursor_mz * buffer.precursor_charge - buffer.precursor_charge * 1.007;
            // filter

            auto mz_int_exist = SetMzIntensity(buffer, current_scan_node);
            if (!mz_int_exist) { current_scan_node = GetNextScan(); continue; }

            // pass all checks, current_scan_node remains the same
            break;
        }
        if (current_scan_node != nullptr) { return true; }
        else { return false; }
    }

private:
    rapidxml::file<> file_;
    rapidxml::xml_document<> doc_;
    std::queue<rapidxml::xml_node<>*> untreated_scan_nodes_;

    // helper functions
    rapidxml::xml_node<>* GetNextScan() {
        if (untreated_scan_nodes_.empty()) { return nullptr; }
        auto next_scan_node = untreated_scan_nodes_.front();
        untreated_scan_nodes_.pop();
        for (auto child_scan_node = next_scan_node->first_node("scan");
                child_scan_node && NodeNameIs(child_scan_node, "scan");
                child_scan_node = child_scan_node->next_sibling()) {
            untreated_scan_nodes_.push(child_scan_node);
        }
        return next_scan_node;
    }

    // builders
    static bool SetParams(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* scan_node) {
        auto scan_num_attr = scan_node->first_attribute("num");
        auto ms_level_attr = scan_node->first_attribute("msLevel");
        auto base_peak_mz_attr = scan_node->first_attribute("basePeakMz");
        auto base_peak_intensity_attr = scan_node->first_attribute("basePeakIntensity");
        auto total_ion_current_attr = scan_node->first_attribute("totIonCurrent");
        if (scan_num_attr == nullptr || ms_level_attr == nullptr || base_peak_mz_attr == nullptr
                || base_peak_intensity_attr == nullptr || total_ion_current_attr == nullptr) {
            return false;
        }
        buffer.scan_num = stoi(GetAttrValue(scan_num_attr));
        buffer.ms_level = stoi(GetAttrValue(ms_level_attr));
        buffer.base_peak_mz = stod(GetAttrValue(base_peak_mz_attr));
        buffer.base_peak_intensity = stod(GetAttrValue(base_peak_intensity_attr));
        buffer.total_ion_current = stod(GetAttrValue(total_ion_current_attr));
        return true;
    }

    static bool SetPrecursorInfo(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* scan_node) {
        auto precursor_mz_node = scan_node->first_node("precursorMz");
        if (precursor_mz_node == nullptr) { return false; }
        auto precursor_charge_attr = precursor_mz_node->first_attribute("precursorCharge");
        if (precursor_charge_attr == nullptr) { return false; }
        buffer.precursor_charge = stoi(GetAttrValue(precursor_charge_attr));
        buffer.precursor_mz = stod(std::string(precursor_mz_node->value(), precursor_mz_node->value_size()));
        return true;
    }

    static bool SetMzIntensity(MzLoader::Spectrum& buffer, rapidxml::xml_node<>* scan_node) {
        auto peaks_node = scan_node->first_node("peaks");
        if (peaks_node == nullptr) { return false; }
        auto precision_attr = peaks_node->first_attribute("precision");
        if (precision_attr == nullptr) { return false; }
        int precision = stoi(GetAttrValue(precision_attr));
        if (precision != 64 && precision != 32) { return false; }

        auto compression_type_attr = peaks_node->first_attribute("compressionType");
        if (compression_type_attr == nullptr) { return false; }
        bool is_compressed;
        if (AttrValueIs(compression_type_attr, "zlib")) {
            is_compressed = true;
        }
        else if (AttrValueIs(compression_type_attr, "none")) {
            is_compressed = false;
        }
        else {  // data error
            return false;
        }

        auto raw_data = peaks_node->value();
        auto raw_data_size = peaks_node->value_size();
        auto decoded_data = DecodeMzData(raw_data, raw_data_size, precision, is_compressed, false);  // big endian
        auto vector_size = decoded_data.size() / 2;
        assert(vector_size * 2 == decoded_data.size());
        vector<std::pair<double, double>> peaks(vector_size);
        for (auto i = 0; i < vector_size; ++i) {
            peaks[i] = std::make_pair(decoded_data[2 * i], decoded_data[2 * i + 1]);
        }
        buffer.peaks = peaks;
        return true;
    }
};
