/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/neutron/neutron.h"

tflite::MicroOpResolver &MODEL_GetOpsResolver()
{
	static tflite::MicroMutableOpResolver<14> s_microOpResolver;
	s_microOpResolver.AddLeakyRelu();
	s_microOpResolver.AddMean();
	s_microOpResolver.AddFullyConnected();
	s_microOpResolver.AddLogistic();
	s_microOpResolver.AddReshape();
	s_microOpResolver.AddMul();
	s_microOpResolver.AddSpaceToBatchNd();
	s_microOpResolver.AddDepthwiseConv2D();
	s_microOpResolver.AddBatchToSpaceNd();
	s_microOpResolver.AddPad();
	s_microOpResolver.AddSlice();
	s_microOpResolver.AddAdd();
	s_microOpResolver.AddReduceMax();
	s_microOpResolver.AddCustom(tflite::GetString_NEUTRON_GRAPH(), tflite::Register_NEUTRON_GRAPH());

    return s_microOpResolver;
}
