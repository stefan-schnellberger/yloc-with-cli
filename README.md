# yloc - A Dynamic Topology Information System

## Introduction

The goal of yloc is to create a dynamic topology information
system that allows for a flexible representation of the hardware
topology of modern architectures by incorporating existing open-source
and vendor-specific data sources. To offer a more general and
widely applicable solution to some of the shortcomings of
existing tools for working with system topology, yloc has
been designed with these principles in mind:

* **Flexible data schema:** Instead of relying on a tree data
  structure, data storage and representation in yloc is based on a
  flexible (multi-)graph representation. Nodes in this multi-graph
  represent components in the hardware setup of a machine, edges may
  represent any labelled relationship between these hardware
  components.


* **Support for multiple data sources:** Since our data
  representation format is very general, the tool has the ability to
  ingest and combine data from a variety of sources. In terms of our
  yloc prototype implementation, the ability to support multiple data
  sources is aided by a module system that makes it comparatively easy
  to add new data sources, such as vendor-specific tools.


* **Basic abstract machine model:** To attach semantic
  meaning to the components of the topology graph (nodes and edges), a
  simple extensible abstract machine model has been developed. This
  model covers the most important aspects of current-generation
  hardware found in high performance computing systems, but is also
  extensible in order to allow it to evolve to cover future hardware
  developments.


* **Unification and integration of data sources:** Since yloc
  allows data from multiple sources to be integrated into a single
  graph representation, the data domains of these sources may be
  overlapping. In order to provide a way to produce a coherently
  integrated data model, yloc identifies nodes that correspond to each
  other in different (sub-)graphs generated by different modules.

* **Support for dynamic data:** In addition to static
  information (hardware components and their relationship or
  connection), it may be often useful to augment this static structure
  with dynamic (time-varying) properties. Dynamic information may
  arise based on time-varying hardware properties, such as temperature
  or operating frequency of compute elements, or it may originate from
  software components, such as an application's dynamic load
  information.

* **Query support:** Having a flexible way to represent data
  in an information system is only a necessary, but not sufficient,
  condition for a useful and practical approach. What is additionally
  needed is a way to extract and find relevant data, i.e., a way to
  formulate and execute queries that can find and filter the
  data. This approach is supported in yloc by means of operations on
  the underlying graph, such as predicate-based graph views and support
  for graph algorithms such as computing shortest paths or performing
  a breadth-first search.

---

## Documentation

### Requirements, Download, Build and Install

`yloc` uses the CMake build system.
In order to configure the software create a build folder and call `cmake <yloc-root>`  from within.

```
git clone <yloc-repo>/yloc.git
cd yloc
mkdir -p build ; cd build
cmake ..
```

CMake will check for dependencies and fail with according messages if they are not met.
Otherwise build files are generated that can be executed either with `make` or `cmake --build .`

By default `yloc` only depends on the Boost Graph Library.
There are further dependencies for the different modules, altough they aren't usually mandatory.
For the moment we strongly recommend `hwloc` as primary source of topology information.

There are CMake options to change the default build of yloc.
To list all available options use `cmake -L` from the build folder.
At the moment there are options to enable or disable specific modules: `ENABLE_<MODULE>`.
They can be set using `cmake -D`:

```
cmake -DENABLE_EXAMPLE=OFF ..
```

### API Reference

TODO

### CLI

The `yloc-cli` executable provides a quick way of filtering hardware information.
There are CLI-Options to: 
* filter component types
* filter component properties
* specify dynamic probing
* specify output format and file

See command `yloc-cli --help` for further information.

---

## yloc Modules

Writing an own module involves implementing two classes: the new module class that implements the YlocModule interface, and the module's adapter class.
The adapter class specifies the list of available properties, and how these properties are accessed in the module.

---

### Module Interface

```CPP
#include <yloc/modules/module.h>

using yloc::Graph;
using yloc::YlocModule;

class ExampleModule : public YlocModule
{

public:
    void init_graph(Graph &graph) override
    {
        return;
    }

    void export_graph(const Graph &graph, void **output) const override
    {
        output = nullptr; return;
    }

    void update_graph(Graph &graph) override
    {
        return;
    }

private:
};

```

### Module Adapter

```CPP
#include <yloc/modules/adapter.h>

class MyAdapter : public yloc::Adapter
{
    using obj_t = MyTopologyObjectType;

public:
    MyAdapter(obj_t obj) : m_obj(obj) {}

    std::optional<std::string> as_string() const override
    {
        /* string representation of object */
    }

    std::optional<uint64_t> memory() const override
    {
        /* implementation for memory property */
    }

    /* [...] */

    std::optional<uint64_t> my_custom_property() const override
    {
        /* implementation for custom property */
    }

    obj_t native_obj() const { return m_obj; }

private:
    obj_t m_obj;
};
```

---

### Supported Topology Information Systems

Yloc implements modules for the following topology information systems:

- [hwloc](https://www.open-mpi.org/projects/hwloc/)
- [ROCm System Management Interface (ROCm SMI) Library](https://github.com/RadeonOpenCompute/rocm_smi_lib)
- [NVIDIA Management Library (NVML)](https://developer.nvidia.com/nvidia-management-library-nvml)


<!--
### Tested Architectures

TODO:
  - tested on ?

---

## Funding

TODO?

-->
