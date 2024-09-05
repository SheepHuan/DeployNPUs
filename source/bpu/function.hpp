#ifndef BPU_FUNCTION_HPP
#define BPU_FUNCTION_HPP
#include <string>
#include <iostream>
#include <cstring>
#include "glog/logging.h"
#include "gflags/gflags.h"
#include <dnn/hb_dnn.h>
#include <cstdio>
#include <iostream>
#include <filesystem>
std::string dump_tensor_shape(hbDNNTensorShape shape)
{
    std::stringstream stream;
    stream << "[";
    for (int i = 0; i < shape.numDimensions; i++)
    {
        if (i == shape.numDimensions - 1)
            stream << shape.dimensionSize[i];
        else
            stream << shape.dimensionSize[i] << ",";
    }
    stream << "]";
    return stream.str();
}

std::string string_tensorlayout(int32_t layout)
{
    switch (layout)
    {
    case hbDNNTensorLayout::HB_DNN_LAYOUT_NHWC:
        /* code */
        return "HB_DNN_LAYOUT_NHWC";
        break;
    case hbDNNTensorLayout::HB_DNN_LAYOUT_NCHW:
        /* code */
        return "HB_DNN_LAYOUT_NCHW";
        break;
    case hbDNNTensorLayout::HB_DNN_LAYOUT_NONE:
        /* code */
        return "HB_DNN_LAYOUT_NONE";
        break;

    default:
        return "HB_DNN_LAYOUT_UNSUPPORTED";
        break;
    }
}

std::string string_tensortype(int32_t type)
{
    switch (type)
    {
    case hbDNNDataType::HB_DNN_IMG_TYPE_Y:
        return "HB_DNN_IMG_TYPE_Y";
        break;
    case hbDNNDataType::HB_DNN_IMG_TYPE_NV12:
        return "HB_DNN_IMG_TYPE_NV12";
        break;
    case hbDNNDataType::HB_DNN_IMG_TYPE_NV12_SEPARATE:
        return "HB_DNN_IMG_TYPE_NV12_SEPARATE";
        break;
    case hbDNNDataType::HB_DNN_IMG_TYPE_YUV444:
        return "HB_DNN_IMG_TYPE_YUV444";
        break;
    case hbDNNDataType::HB_DNN_IMG_TYPE_RGB:
        return "HB_DNN_IMG_TYPE_RGB";
        break;
    case hbDNNDataType::HB_DNN_IMG_TYPE_BGR:
        return "HB_DNN_IMG_TYPE_BGR";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_S4:
        return "HB_DNN_TENSOR_TYPE_S4";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_U4:
        return "HB_DNN_TENSOR_TYPE_U4";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_S8:
        return "HB_DNN_TENSOR_TYPE_S8";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_U8:
        return "HB_DNN_TENSOR_TYPE_U8";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_F16:
        return "HB_DNN_TENSOR_TYPE_F16";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_S16:
        return "HB_DNN_TENSOR_TYPE_S16";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_U16:
        return "HB_DNN_TENSOR_TYPE_U16";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_F32:
        return "HB_DNN_TENSOR_TYPE_F32";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_S32:
        return "HB_DNN_TENSOR_TYPE_S32";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_U32:
        return "HB_DNN_TENSOR_TYPE_U32";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_F64:
        return "HB_DNN_TENSOR_TYPE_F64";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_S64:
        return "HB_DNN_TENSOR_TYPE_S64";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_U64:
        return "HB_DNN_TENSOR_TYPE_U64";
        break;
    case hbDNNDataType::HB_DNN_TENSOR_TYPE_MAX:
        return "HB_DNN_TENSOR_TYPE_MAX";
        break;

    default:
        return "HB_DNN_TENSOR_UNSUPPORTED";
        break;
    }
}

void dump_tensor_properties(int index, std::string name, hbDNNTensorProperties prop, bool is_input = true)
{
    std::string tensor_type = is_input ? "input tensor" : "output tensor";
    LOG(INFO) << tensor_type << ", index = " << index << ", name = " << name << ", valid shape = " << dump_tensor_shape(prop.validShape) << ", aligned shape = " << dump_tensor_shape(prop.alignedShape) << ", tensor layout = " << string_tensorlayout(prop.tensorLayout) << ", tensor type = " << string_tensortype(prop.tensorType);
}

#endif