# Copyright (c) 2026 TXCompute. Licensed under the MIT License.

idf_component_register(
	SRCS
		"include/tx/csl_engine.h"
		"include/impl/expression_evaluator.hpp"
	INCLUDE_DIRS
		"include"
	REQUIRES
		"TXLib"
)