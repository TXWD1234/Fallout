# Copyright (c) 2026 TXCompute. Licensed under the MIT License.

idf_component_register(
	SRCS
		"include/tx/addons.h"
		"include/impl/handle_system.hpp"
	INCLUDE_DIRS
		"include"
	REQUIRES
		"TXLib"
)