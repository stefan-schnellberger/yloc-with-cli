
#include <interface.h>

#include "init_graph.h"

// enum aliases for backwards compatibility from hwloc documentation
#if HWLOC_API_VERSION < 0x00010b00
#define HWLOC_OBJ_NUMANODE HWLOC_OBJ_NODE
#define HWLOC_OBJ_PACKAGE HWLOC_OBJ_SOCKET
#endif /* HWLOC_API_VERSION */

using yloc::graph_t;
using yloc::YlocModule;

// TODO: separate definition from declaration, remove init_graph_myloq

class YlocHwloc : public YlocModule
{
public:
    void init_graph(graph_t * graph) {               // init complete graph
        // Todo proper lifetime of this object
        *graph = init_graph_myloq("0");
        return;
    }

    void init_graph_secondary(graph_t * graph) {     // init of secondary module (if graph already is initialized by another module)
        return;
    }

    // Specifies wheter init_graph is implemented and if the module can be used standalone // todo better comment
    // Todo better module capability system
    bool is_main_module() {
        return true;
    }

    // optional function, not ever module requires this
    // it is recommended that every module that can init a graph on its own also provides this function.
    // Todo return value for feedback if export was successful/supported
    void export_graph(graph_t * graph, void ** output) {
        output = nullptr;
        return;
    }

    void update_graph(graph_t * graph) {
        return;
    };

private:
};
