#pragma once

#ifndef YLOC_CLI_H
#define YLOC_CLI_H

#endif // YLOC_CLI_H

#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp> // print_graph
#include <boost/graph/graphviz.hpp>      // write_graphviz
#include <cstdio>
#include <chrono>
#include <ctime>
#include <getopt.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <yloc/modules/module.h>
#include <yloc/modules/adapter.h>

#include "yloc/yloc.h"

/**
 * The Default behaviour of getopt_long is, that a optional argument must follow the option-Character directly (e.g. -d8000).
 * From a user perspective optional arguments should be treated like required arguments (e.g. -d 8000 should be possible).
 * In order to achieve such behaviour we must define a helper macro.
 * Compare: https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
 * Code taken from https://github.com/NorthernTechHQ/libntech
 */
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))

using namespace yloc;

static struct option long_options[] =
    {
        {"help", no_argument, nullptr, 'h'},
        {"list-modules", no_argument, nullptr, 'M'},
        {"list-component-types", no_argument, nullptr, 'C'},
        {"list-properties-per-component", no_argument, nullptr, 'P'},
        {"list-output-formats", no_argument, nullptr, 'O'},
        {"component-types", required_argument, nullptr, 'c'},
        {"vertex-properties", required_argument, nullptr, 'p'},
        {"dynamic-probing", optional_argument, nullptr, 'd'},
        {"probing-period", required_argument, nullptr, 'l'},
        {"output", required_argument, nullptr, 'o'},
        {"output-format", required_argument, nullptr, 'f'},
        {nullptr, 0, nullptr, 0}
};

template < class Name > class csv_writer
{
public:
    explicit csv_writer(Name _name) : name(_name) {}
    template < class VertexOrEdge >
    void operator()(std::ostream& out, const VertexOrEdge& v) const
    {
        out << get(name, v);
    }

private:
    Name name;
};

// Default Constants
const std::array<std::string, 2> VALID_OUTPUT_FORMATS = {"dot", "csv"};
const std::vector<std::string> DEFAULT_VECTOR_PROPERTIES = {"memory", "numa_affinity"};
const int DEFAULT_PROBING_FREQUENCY = 1000; // in milliseconds [ms]
const int DEFAULT_PROBING_PERIOD = 60000; // in milliseconds [ms]

/**
 * compare: https://stackoverflow.com/a/35157784
 * @return
 */
std::string date_time_in_milliseconds()
{
    using namespace std::chrono;

    // get current time
    auto now = system_clock::now();

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = system_clock::to_time_t(now);

    // convert to broken time
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    oss << std::put_time(&bt, "%F %T"); // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

void show_help()
{
    printf("Usage: yloc-cli [OPTION]...\n"
           "Get a graph representation of the hardware topology of your system.\n"
           "If no additional OPTION is given, the command will print\n"
           "a dot-format graph representation of the host machine to stdout.\n"
           "\n"
           "Mandatory arguments to long options are mandatory for short options too.\n"
           "\n"
           "  -M, --list-modules        list available modules and exit;\n"
           "                            modules can provide additional hardware information,\n"
           "                            see <https://github.com/sparcityeu/yloc#yloc-modules>\n"
           "  -C, --list-component-types\n"
           "                            list available component types and exit;\n"
           "                            based on the abstract machine model at ylocs core;\n"
           "                            listed component types can be used to filter\n"
           "                            specific information, see option -c\n"
           "  -P, --list-properties-per-component\n"
           "                            list properties per component type and exit;\n"
           "                            based on the abstract machine model at ylocs core;\n"
           "                            listed properties can be used to filter\n"
           "                            specific information, see option -p\n"
           "  -O, --list-output-formats\n"
           "                            list available formats and exit;\n"
           "                            listed output-formats can be used as\n"
           "                            file extension for option -o\n"
           "                            and also as argument for option -f\n"
           "\n"
           "  -c, --components=COMPONENT1,COMPONENT2\n"
           "                            only include listed component-types in resulting graph\n"
           "  -p, --vertex-properties=PROPERTY1,PROPERTY2\n"
           "                            only include listed properties in vertices of resulting graph\n"
           "  -d, --dynamic-probing[=PROBING-FREQUENCY]\n"
           "                            periodically probe hardware to see dynamic changes;\n"
           "                            optional PROBING-FREQUENCY must be in milliseconds\n"
           "                            (default: 1000 ms);\n"
           "                            process will keep running until the time limit\n"
           "                            of the probing-period is reached (default 60000 ms),\n"
           "                            custom limit for probing-period can be set with option -l\n"
           "  -l, --probing-period=LIMIT\n"
           "                            sets a custom time limit for the probing period;\n"
           "                            LIMIT must be in milliseconds (default 60000 ms);\n"
           "                            only has an effect when used together with option -d;\n"
           "\n"
           "  -o, --output=FILE.EXTENSION\n"
           "                            specify output file;\n"
           "                            for a list of supported file extensions see option -O;\n"
           "                            if -o is used without option -f, a given EXTENSION\n"
           "                            will also determine output-format;\n"
           "                            if FILE has no EXTENSION, format will default to dot\n"
           "  -f, --output-format=FORMAT\n"
           "                            specify output format;\n"
           "                            see list of supported output formats with option -O;\n"
           "                            if option -o and -f are used together,\n"
           "                            -f will determine format regardless of file-extension\n"
           "  -h, --help        display this help and exit\n"
           "\n"
           "Further information: <https://github.com/sparcityeu/yloc>\n"
    );
}

void show_output_formats()
{
    printf("Valid output formats to use as argument for option -f.\n"
           "Can also be used as extension for the output-file specified with option -o.\n"
           "See options -o and -f  in `yloc-cli --help`.\n"
           "\n"
           "  dot               graph description language of the Graphviz project\n"
           "                    (see <https://en.wikipedia.org/wiki/Graphviz>),\n"
           "                    well suited for further visualisation\n"
           "  csv               comma separated values, stores tabular data as plain text;\n"
           "                    well suited for further analysis with spreadsheet applications, databases\n"
           "                    or Python and R\n"
           "\n"
           "not yet implemented (contact authors of GitHub-Repo if needed):\n"
           "  json              JavaScript Object Notation, stores data as key-value pairs and arrays;\n"
           "                    well suited for data interchange between different programming languages\n"
    );
}