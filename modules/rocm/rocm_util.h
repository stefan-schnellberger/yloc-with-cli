#pragma once
#include <rocm_smi/rocm_smi.h>

#define CHECK_ROCM(x) ((x) == RSMI_STATUS_SUCCESS)

#define CHECK_ROCM_MSG(x)                                                                                    \
    ({                                                                                                       \
        bool success;                                                                                        \
        rsmi_status_t ret = (x);                                                                             \
        success = (ret == RSMI_STATUS_SUCCESS);                                                              \
        if (!success) {                                                                                      \
            const char *err_msg;                                                                             \
            rsmi_status_string(ret, &err_msg);                                                               \
            std::cerr << "[yloc ROCm module] " #x " failed in file " __FILE__ " line " << __LINE__ << ":\n"; \
            std::cerr << err_msg << '\n';                                                                    \
        }                                                                                                    \
        success;                                                                                             \
    })

#define EXIT_ERR_ROCM(x)                            \
    do {                                            \
        if (!CHECK_ROCM_MSG(x)) {                   \
            std::cerr << "shutting rsmi down...\n"; \
            rsmi_shut_down();                       \
        }                                           \
    } while (0)

static inline const char *yloc_rocm_link_type_str(RSMI_IO_LINK_TYPE link_type)
{
    switch (link_type) {
    case RSMI_IOLINK_TYPE_PCIEXPRESS:
        return "RSMI_IOLINK_TYPE_PCIEXPRESS";

    case RSMI_IOLINK_TYPE_XGMI:
        return "RSMI_IOLINK_TYPE_XGMI";

    default:
        return "RSMI_IOLINK_TYPE_UNDEFINED";
    }
}

void yloc_rocm_get_supported_functions(uint32_t num_devices)
{
    rsmi_func_id_iter_handle_t iter_handle, var_iter, sub_var_iter;
    rsmi_func_id_value_t value;
    rsmi_status_t err;
    for (uint32_t i = 0; i < num_devices; ++i) {
        std::cout << "Supported RSMI Functions:" << std::endl;
        std::cout << "\tVariants (Monitors)" << std::endl;
        err = rsmi_dev_supported_func_iterator_open(i, &iter_handle);
        while (1) {
            err = rsmi_func_iter_value_get(iter_handle, &value);
            std::cout << "Function Name: " << value.name << std::endl;
            err = rsmi_dev_supported_variant_iterator_open(iter_handle, &var_iter);
            if (err != RSMI_STATUS_NO_DATA) {
                std::cout << "\tVariants/Monitors: ";
                while (1) {
                    err = rsmi_func_iter_value_get(var_iter, &value);
                    if (value.id == RSMI_DEFAULT_VARIANT) {
                        std::cout << "Default Variant ";
                    } else {
                        std::cout << value.id;
                    }
                    std::cout << " (";
                    err =
                        rsmi_dev_supported_variant_iterator_open(var_iter, &sub_var_iter);
                    if (err != RSMI_STATUS_NO_DATA) {
                        while (1) {
                            err = rsmi_func_iter_value_get(sub_var_iter, &value);
                            std::cout << value.id << ", ";
                            err = rsmi_func_iter_next(sub_var_iter);
                            if (err == RSMI_STATUS_NO_DATA) {
                                break;
                            }
                        }
                        err = rsmi_dev_supported_func_iterator_close(&sub_var_iter);
                    }
                    std::cout << "), ";
                    err = rsmi_func_iter_next(var_iter);
                    if (err == RSMI_STATUS_NO_DATA) {
                        break;
                    }
                }
                std::cout << std::endl;
                err = rsmi_dev_supported_func_iterator_close(&var_iter);
            }
            err = rsmi_func_iter_next(iter_handle);
            if (err == RSMI_STATUS_NO_DATA) {
                break;
            }
        }
        err = rsmi_dev_supported_func_iterator_close(&iter_handle);
    }
}