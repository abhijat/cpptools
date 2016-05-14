#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;

namespace return_code
{
    const int SUCCESS = 0;
    const int FAILURE_PARSE = 1;
    const int FAILURE_CREATE_FILE = 2;
    const int FAILURE_FILE_EXISTS = 3;
}

bool check_file_can_be_created(const std::string& a, const std::string& b)
{
    return !boost::filesystem::exists(a) && !boost::filesystem::exists(b);
}

void build_header(const std::string& f, const std::string& ext, const po::variables_map& vm)
{
    std::ofstream ofs(f);
    if (!ofs) {
        throw std::runtime_error("failed to create header");
    }

    auto ns = vm["namespace"].as<std::string>();
    auto cls = vm["class"].as<std::string>();

    boost::to_upper(ns);
    boost::to_upper(cls);

    std::string e(ext);
    boost::to_upper(e);

    ofs << "#ifndef " << ns << "_" << cls << "_" << e << "\n";
    ofs << "#define " << ns << "_" << cls << "_" << e << "\n";
    ofs << "\n\n";
    ofs << "namespace " << vm["namespace"].as<std::string>() << "\n";
    ofs << "{\n\n\n}\n\n#endif\n\n";
    
    ofs.close();
}

void build_source(const std::string& s, const std::string& h, const po::variables_map& vm)
{
    std::ofstream ofs(s);
    if (!ofs) {
        throw std::runtime_error("failed to create source");
    }
    
    ofs << "#include \"" << h << "\";\n";
    ofs << "\n" << "namespace " << vm["namespace"].as<std::string>() << "\n";
    ofs << "{\n\n\n}\n\n";

    ofs.close();
}

int process_options(const po::variables_map& vm)
{
    std::string ns = vm["namespace"].as<std::string>();
    std::string fn = vm["class"].as<std::string>();

    std::string header_name;
    std::string source_name;
    std::string header_extension;

    auto style = vm["style"].as<std::string>();
    if (style == "cc") {
        header_extension = "h";
        header_name = fn + "." + header_extension;
        source_name = fn + ".cc";
    } else if (style == "cxx") {
        header_extension = "hxx";
        header_name = fn + "." + header_extension;
        source_name = fn + ".cxx";
    } else {
        header_extension = "hpp";
        header_name = fn + "." + header_extension;
        source_name = fn + ".cpp";
    }

    if (!check_file_can_be_created(header_name, source_name)) {
        std::cerr << "one of the files exists\n";
        return return_code::FAILURE_FILE_EXISTS;
    }

    try {
        build_header(header_name, header_extension, vm);
        build_source(source_name, header_name, vm);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create files " << e.what() << std::endl;
        return return_code::FAILURE_CREATE_FILE;
    }

    return return_code::SUCCESS;
}

int main(int argc, char* argv[])
{
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "show this help message")
        ("namespace,n", po::value<std::string>(), "namespace, used to build header guards")
        ("class,c",  po::value<std::string>(), "header and source file name, class name")
        ("style,s", po::value<std::string>()->default_value("cpp"), "filename style: cc/cxx/cpp h/hxx/hpp");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help") || !vm.count("namespace") || !vm.count("class")) {
            std::cout << "class and header generator usage: \n" << desc << std::endl;
            return return_code::SUCCESS;
        }

        po::notify(vm);
        process_options(vm);

    } catch (const po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return return_code::FAILURE_PARSE;
    }

    return return_code::SUCCESS;
}
