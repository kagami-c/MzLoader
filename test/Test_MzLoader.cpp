#include "Decode.h"  // internal header
#include "MzLoader.h"
#include <gtest/gtest.h>
#include <vector>

#define alloc_func rapidxml_alloc_func
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#undef alloc_func

using std::vector;

TEST(Unittest_MzLoader, Uncompressed64bitMzml) {
    rapidxml::file<> mzml_file("small.pwiz.1.1.mzML");  // uncompressed 64-bit and 32-bit mixture
    rapidxml::xml_document<> doc;
    doc.parse<0>(mzml_file.data());
    auto spectrum_node = doc.first_node("indexedmzML")->first_node("mzML")->first_node("run")->first_node("spectrumList")->first_node("spectrum");
    auto binary_data_array_node = spectrum_node->first_node("binaryDataArrayList")->first_node("binaryDataArray");
    auto mz_data_node = binary_data_array_node->first_node("binary");

    auto mz_array = mz_data_node->value();
    auto mz_array_size = mz_data_node->value_size();
    auto v = DecodeMzData(mz_array, mz_array_size, 64, false, true);

    std::ifstream test_file("mz_list.test");
    assert(test_file.is_open());
    for (auto decoded_double : v) {
        float reference_value;
        test_file >> reference_value;
        EXPECT_FLOAT_EQ(reference_value, decoded_double);
    }
    EXPECT_TRUE(test_file.eof());
}

TEST(Unittest_MzLoader, Compressed64bitMzml) {
    rapidxml::file<> mzml_file("small_raw_compressed.mzML");  // compressed 64-bit
    rapidxml::xml_document<> doc;
    doc.parse<0>(mzml_file.data());
    auto spectrum_node = doc.first_node("indexedmzML")->first_node("mzML")->first_node("run")->first_node("spectrumList")->first_node("spectrum");
    auto binary_data_array_node = spectrum_node->first_node("binaryDataArrayList")->first_node("binaryDataArray");
    auto mz_data_node = binary_data_array_node->first_node("binary");

    auto mz_array = mz_data_node->value();
    auto mz_array_size = mz_data_node->value_size();
    auto v = DecodeMzData(mz_array, mz_array_size, 64, true, true);

    std::ifstream test_file("mz_list.test");
    assert(test_file.is_open());
    for (auto decoded_double : v) {
        float reference_value;
        test_file >> reference_value;
        EXPECT_FLOAT_EQ(reference_value, decoded_double);
    }
    EXPECT_TRUE(test_file.eof());
}

TEST(Unittest_MzLoader, Uncompressed32BitMzml) {
    rapidxml::file<> mzml_file("small.pwiz.1.1.mzML");  // uncompressed 64-bit and 32-bit mixture
    rapidxml::xml_document<> doc;
    doc.parse<0>(mzml_file.data());
    auto spectrum_node = doc.first_node("indexedmzML")->first_node("mzML")->first_node("run")->first_node("spectrumList")->first_node("spectrum");
    auto binary_data_array_node = spectrum_node->first_node("binaryDataArrayList")->first_node("binaryDataArray")->next_sibling();
    auto int_data_node = binary_data_array_node->first_node("binary");

    auto int_array = int_data_node->value();
    auto int_array_size = int_data_node->value_size();
    auto v = DecodeMzData(int_array, int_array_size, 32, false, true);

    std::ifstream test_file("int_list.test");
    assert(test_file.is_open());
    for (auto decoded_double : v) {
        float reference_value;
        test_file >> reference_value;
        EXPECT_FLOAT_EQ(reference_value, decoded_double);
    }
    EXPECT_TRUE(test_file.eof());
}

TEST(Unittest_MzLoader, Compressed32BitMzml) {
    rapidxml::file<> mzml_file("small_zlib.pwiz.1.1.mzML");  // compressed 32-bit
    rapidxml::xml_document<> doc;
    doc.parse<0>(mzml_file.data());
    auto spectrum_node = doc.first_node("indexedmzML")->first_node("mzML")->first_node("run")->first_node("spectrumList")->first_node("spectrum");
    auto binary_data_array_node = spectrum_node->first_node("binaryDataArrayList")->first_node("binaryDataArray")->next_sibling();
    auto int_data_node = binary_data_array_node->first_node("binary");

    auto int_array = int_data_node->value();
    auto int_array_size = int_data_node->value_size();
    auto v = DecodeMzData(int_array, int_array_size, 32, true, true);

    std::ifstream test_file("int_list.test");
    assert(test_file.is_open());
    for (auto decoded_double : v) {
        float reference_value;
        test_file >> reference_value;
        EXPECT_FLOAT_EQ(reference_value, decoded_double);
    }
    EXPECT_TRUE(test_file.eof());
}

// real data validation, PASS!
TEST(Unittest_MzLoader, MzmlAPI) {
    MzLoader loader("OR20080527_S_mix8FTFTprof-prof_03.1294.mzML");
    std::ifstream test_file("OR20080527_S_mix8FTFTprof-prof_03.1294.test");
    MzLoader::Spectrum spectrum;
    while (loader.LoadNext(spectrum)) {
        for (auto peak : spectrum.peaks) {
            float reference_mz;
            test_file >> reference_mz;
            EXPECT_FLOAT_EQ(reference_mz, peak.first);
        }
        for (auto peak : spectrum.peaks) {
            float reference_int;
            test_file >> reference_int;
            EXPECT_FLOAT_EQ(reference_int, peak.second);
        }
    }
    EXPECT_TRUE(test_file.eof());
}

TEST(Unittest_MzLoader, MzxmlAPI) {
    MzLoader loader("OR20080527_S_mix8FTFTprof-prof_03.mzXML");
    std::ifstream test_file("OR20080527_S_mix8FTFTprof-prof_03.1294.test");
    MzLoader::Spectrum spectrum;
    while (loader.LoadNext(spectrum)) {
        for (auto peak : spectrum.peaks) {
            float reference_mz;
            test_file >> reference_mz;
            EXPECT_FLOAT_EQ(reference_mz, peak.first);
        }
        for (auto peak : spectrum.peaks) {
            float reference_int;
            test_file >> reference_int;
            EXPECT_FLOAT_EQ(reference_int, peak.second);
        }
    }
    EXPECT_TRUE(test_file.eof());
}

// To test all paths, there are 2^3 settings of input format.
// Please try a validation before using this library.
// All little endian settings are tested.