#include "ZipWriter.h"

#include "DslWriter.h"
#include "dictlsd/lsd.h"
#include "dictlsd/tools.h"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <map>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace dictlsd;

struct Entry {
    std::u16string heading;
    std::u16string article;
};

unsigned getFileSize(std::fstream& file) {
    unsigned pos = file.tellg();
    file.seekg(0, std::ios_base::end);
    unsigned size = file.tellg();
    file.seekg(pos, std::ios_base::beg);
    return size;
}

template <typename T>
std::vector<T>& append(std::vector<T>& vec, std::vector<T> const& rhs) {
    std::copy(rhs.begin(), rhs.end(), std::back_inserter(vec));
    return vec;
}

int parseLSD(fs::path lsdPath,
             fs::path outputPath,
             int sourceFilter,
             int targetFilter,
             std::ostream& log)
{
    std::fstream lsd(lsdPath.string(), std::ios::in | std::ios::binary);
    if (!lsd.is_open())
        throw std::runtime_error("Can't open the LSD file.");

    unsigned fileSize = getFileSize(lsd);
    std::unique_ptr<char[]> buf(new char[fileSize]);
    lsd.read(buf.get(), fileSize);

    InMemoryStream ras(buf.get(), fileSize);
    BitStreamAdapter bstr(&ras);
    LSDDictionary reader(&bstr);
    LSDHeader header = reader.header();
    log << "Header:";
    log << "\n  Version:  " << std::hex << header.version << std::dec;
    log << "\n  Entries:  " << header.entriesCount;
    log << "\n  Source:   " << header.sourceLanguage
        << " (" << toUtf8(langFromCode(header.sourceLanguage)) << ")";
    log << "\n  Target:   " << header.targetLanguage
        << " (" << toUtf8(langFromCode(header.targetLanguage)) << ")";
    log << "\n  Name:     " << toUtf8(reader.name()) << std::endl;

    if ((sourceFilter != -1 && sourceFilter != header.sourceLanguage) ||
        (targetFilter != -1 && targetFilter != header.targetLanguage))
    {
        log << "ignoring\n";
        return 0;
    }

    writeDSL(&reader, lsdPath.filename().string(), outputPath.string(),
             [&](int, std::string s) { log << s << std::endl; });

    return 0;
}

int main(int argc, char* argv[]) {
    std::string lsdPath, outputPath;
    int sourceFilter = -1, targetFilter = -1;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value<std::string>(&lsdPath), "LSD dictionary to decode")
            ("source-filter", po::value<int>(&sourceFilter),
                "ignore dictionaries with source language != source-filter")
            ("target-filter", po::value<int>(&targetFilter),
                "ignore dictionaries with target language != target-filter")
            ("codes", "print supported languages and their codes")
            ("out", po::value<std::string>(&outputPath)->required(), "output directory")
            ;
        po::variables_map console_vm;
        po::store(po::parse_command_line(argc, argv, console_desc), console_vm);
        if (console_vm.count("help")) {
            std::cout << console_desc;
            return 0;
        }
        if (console_vm.count("codes")) {
            printLanguages(std::cout);
            return 0;
        }
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    try {
        fs::create_directories(outputPath);
        if (!lsdPath.empty()) {
            parseLSD(lsdPath,
                     outputPath,
                     sourceFilter,
                     targetFilter,
                     std::cout);
        }
    } catch (std::exception& exc) {
        std::cout << "an error occured while processing dictionary: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}