
#include <adapter.h>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_utility.hpp> // print_graph
#include <boost/graph/graphviz.hpp>      // write_graphviz
#include <iostream>

#include <interface.h>

#include "interface_impl.h"
#include "rocm_adapter.h"
#include "rocm_util.h"

#define VERBOSE_ROCM 0

using namespace yloc;

static void print_bdfid(uint64_t bdfid)
{
    // BDFID = ((DOMAIN & 0xffffffff) << 32) | ((BUS & 0xff) << 8) |
    // ((DEVICE & 0x1f) <<3 ) | (FUNCTION & 0x7)
    uint domain = static_cast<uint>((bdfid >> 32) & 0xffffffffULL);
    uint bus = static_cast<uint>((bdfid >> 8) & 0xffULL);
    uint device = static_cast<uint>((bdfid >> 3) & 0x1fULL);
    uint function = static_cast<uint>(bdfid & 0x7ULL);
    std::cout << std::hex << bdfid << "::" << domain << ":" << bus << ":" << device << ":" << function << std::dec << '\n';
}

void YlocRocm::init_graph()
{
#if USE_SUBGRAPH
    auto g = m_subgraph;
#else
    auto g = root_graph();
#endif

    EXIT_ERR_ROCM(rsmi_init(0)); /* RSMI_INIT_FLAG_ALL_GPUS */
    uint32_t num_devices;
    uint16_t dev_id;
    uint64_t bdfid; // PCIe bus device function id
    CHECK_ROCM_MSG(rsmi_num_monitor_devices(&num_devices));
    std::cout << "num rocm devices: " << num_devices << '\n';

    std::vector<vertex_descriptor_t> vertices(num_devices);
    // std::cout << vertices.size() << '\n';

    if (VERBOSE_ROCM) {
        std::cout << "rocm supported functions:" << '\n';
        yloc_rocm_get_supported_functions(num_devices);
    }

    // associate rocm device with graph node by pcie bdfid:
    for (uint32_t dev_index = 0; dev_index < num_devices; ++dev_index) {
        //  auto gpu_vd = boost::add_vertex(g);
        CHECK_ROCM_MSG(rsmi_dev_id_get(dev_index, &dev_id));

        CHECK_ROCM_MSG(rsmi_dev_pci_id_get(dev_index, &bdfid));
        std::cout << "GPU BDFID=" << bdfid << '\n';

        uint32_t numa_node;
        CHECK_ROCM_MSG(rsmi_topo_numa_affinity_get(dev_index, &numa_node));
        std::cout << "GPU NUMA AFFINITY=" << numa_node << '\n';

        uint64_t memory_total;
        CHECK_ROCM_MSG(rsmi_dev_memory_total_get(dev_index, RSMI_MEM_TYPE_VRAM, &memory_total));
        std::cout << "GPU MEMORY VRAM=" << memory_total << '\n';
        CHECK_ROCM_MSG(rsmi_dev_memory_total_get(dev_index, RSMI_MEM_TYPE_VIS_VRAM, &memory_total));
        std::cout << "GPU MEMORY VISIBLE VRAM=" << memory_total << '\n';
        CHECK_ROCM_MSG(rsmi_dev_memory_total_get(dev_index, RSMI_MEM_TYPE_GTT, &memory_total));
        std::cout << "GPU MEMORY GTT=" << memory_total << '\n';

        auto fgv = boost::make_filtered_graph(g, boost::keep_all{}, [&](const vertex_descriptor_t &v) -> bool {
            return g[v].tinfo.type->is_a<PCIDevice>(); // or Bridge?!
        });
        for (auto vd : boost::make_iterator_range(boost::vertices(fgv))) {
            auto vd_bdfid = YLOC_GET(g, vd, bdfid);
            if (vd_bdfid.has_value() && vd_bdfid.value() == bdfid) {
                std::cout << "GPU found at vd=" << vd << " BDFID=" << vd_bdfid.value() << '\n';
                std::cout << "adding ROCm adapter...\n";
                g[vd].tinfo.push_back(new RocmAdapter{dev_index});
                vertices[dev_index] = vd;
                break;
            }
        }
    }

    // get connectivity and topology between devices: (assuming symmetric connection)
    for (uint32_t dev_ind_src = 0; dev_ind_src < num_devices; ++dev_ind_src) {
        for (uint32_t dev_ind_dst = dev_ind_src + 1; dev_ind_dst < num_devices; ++dev_ind_dst) {
            bool p2p_accessible;
            CHECK_ROCM_MSG(rsmi_is_P2P_accessible(dev_ind_src, dev_ind_dst, &p2p_accessible));
            std::cout << "device " << dev_ind_src << " and " << dev_ind_dst << " are P2P " << (p2p_accessible ? "accessible" : "not accessible") << '\n';

            // get link type
            RSMI_IO_LINK_TYPE link_type;
            uint64_t hops;
            CHECK_ROCM_MSG(rsmi_topo_get_link_type(dev_ind_src, dev_ind_dst, &hops, &link_type));
            std::cout << "link type: " << yloc_rocm_link_type_str(link_type) << " hops: " << hops << '\n';

            uint64_t weight;
            // Retrieve the weight for a connection between 2 GPUs.
            CHECK_ROCM_MSG(rsmi_topo_get_link_weight(dev_ind_src, dev_ind_dst, &weight));
            std::cout << "link weight: " << weight << '\n';

            // Retreive minimal and maximal theoretical io link bandwidth between 2 GPUs.
            // API works if src and dst are connected via xgmi and have 1 hop distance.
            uint64_t min_bandwidth;
            uint64_t max_bandwidth;
            if (link_type == RSMI_IOLINK_TYPE_XGMI) {
                CHECK_ROCM_MSG(rsmi_minmax_bandwidth_get(dev_ind_src, dev_ind_dst, &min_bandwidth, &max_bandwidth));
            }

            if (p2p_accessible) {
                /** TODO: pass subgraph instead of root graph? */
                std::cout << "link gpu indices: " << dev_ind_src << " <-> " << dev_ind_dst << '\n';
                std::cout << "link graph vds: " << vertices[dev_ind_src] << " <-> " << vertices[dev_ind_dst] << '\n';

                /** TODO: std::bald_alloc if add_edge */
#if USE_SUBGRAPH
                // auto ret = boost::add_edge(vertices[dev_ind_src], vertices[dev_ind_dst], graph_t::edge_property_type{0, Edge{edge_type::YLOC_GPU_INTERCONNECT}}, g);
                // ret = boost::add_edge(vertices[dev_ind_dst], vertices[dev_ind_src], graph_t::edge_property_type{0, Edge{edge_type::YLOC_GPU_INTERCONNECT}}, g);
#else
                auto ret = boost::add_edge(vertices[dev_ind_src], vertices[dev_ind_dst], Edge{edge_type::YLOC_GPU_INTERCONNECT}, g);
                ret = boost::add_edge(vertices[dev_ind_dst], vertices[dev_ind_src], Edge{edge_type::YLOC_GPU_INTERCONNECT}, g);
#endif
            }
        }
    }
    return;
}

#if 0
/************************************************/
        Hardware Topology Functions
/************************************************/

rsmi_status_t rsmi_minmax_bandwidth_get (
uint32_t dv_ind_src,
uint32_t dv_ind_dst,
uint64_t * min_bandwidth,
uint64_t * max_bandwidth )
Retreive minimal and maximal io link bandwidth between 2 GPUs.
Given a source device index dv_ind_src and a destination device index dv_ind_dst , pointer to an uint64_t
min_bandwidth , and a pointer to uint64_t max_bandiwidth , this function will write theoretical minimal and
maximal bandwidth limits. API works if src and dst are connected via xgmi and have 1 hop distance.

/** TODO: link bandwidth / throughput */

rsmi_dev_pci_bandwidth_get (uint32_t dv_ind, rsmi_pcie_bandwidth_t * bandwidth)
Get the list of possible PCIe bandwidths that are available.

rsmi_frequencies_t transfer_rate = bandwidth.transfer_rate;
uint32_t rsmi_pcie_bandwidth_t::lanes[RSMI_MAX_NUM_FREQUENCIES]

All bits with indices greater than or equal to the value of the rsmi_frequencies_t::num_supported field

rsmi_frequencies_t Data Fields
* uint32_t num_supported
* uint32_t current The current frequency index
* uint64_t frequency [RSMI_MAX_NUM_FREQUENCIES] 


rsmi_status_t rsmi_dev_pci_throughput_get (
uint32_t dv_ind,
uint64_t * sent,
uint64_t * received,
uint64_t * max_pkt_sz )
Give a device index dv_ind and pointers to a uint64_t s, sent , received and max_pkt_sz , this function will
write the number of bytes sent and received in 1 second to sent and received , respectively. The maximum
possible packet size will be written to max_pkt_sz .


rsmi_status_t rsmi_utilization_count_get (
uint32_t dv_ind,
rsmi_utilization_counter_t utilization_counters[ ],
uint32_t count,
uint64_t * timestamp )
Get coarse grain utilization counter of the specified device.
Given a device index dv_ind , the array of the utilization counters, the size of the array, this function returns the
coarse grain utilization counters and timestamp. The counter is the accumulated percentages. Every milliseconds
the firmware calculates % busy count and then accumulates that value in the counter. This provides minimally
invasive coarse grain GPU usage information.


rsmi_status_t rsmi_dev_perf_level_get (
uint32_t dv_ind,
rsmi_dev_perf_level_t * perf )
Get the performance level of the device with provided device index.
Given a device index dv_ind and a pointer to a uint32_t perf , this function will write the rsmi_dev_perf_level_t
to the uint32_t pointed to by perf

rsmi_status_t rsmi_dev_gpu_metrics_info_get (
uint32_t dv_ind,
rsmi_gpu_metrics_t * pgpu_metrics )
This function retrieves the gpu metrics information.
Given a device index dv_ind and a pointer to a rsmi_gpu_metrics_t structure pgpu_metrics , this function will
populate pgpu_metrics . See rsmi_gpu_metrics_t for more details.


<< Performance Counter Functions REQUIRED IN YLOC? >>
rsmi_counter_available_counters_get


* rsmi_status_t rsmi_compute_process_info_get (rsmi_process_info_t * procs, uint32_t * num_items)
Get process information about processes currently using GPU.
* rsmi_status_t rsmi_compute_process_info_by_pid_get (uint32_t pid, rsmi_process_info_t * proc)
Get process information about a specific process.
* rsmi_status_t rsmi_compute_process_gpus_get (uint32_t pid, uint32_t * dv_indices, uint32_t * num_devices)
Get the device indices currently being used by a process.

#endif
