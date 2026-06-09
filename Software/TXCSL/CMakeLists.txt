# Copyright (c) 2026 TXCompute. Licensed under the MIT License.

idf_component_register(
	SRCS
		"include/tx/csl.h"
		"include/impl/csl/definition.hpp"
		"include/impl/csl/compiler.hpp"
		"include/impl/csl/evaluator.hpp"
		"include/impl/csl/compiler_bracket_parser.hpp"
		"include/impl/csl/compiler_tokenlizer.hpp"
		"include/impl/csl/compiler_codegen.hpp"

		"src/compiler.cpp"
		"src/compiler_tokenlizer.cpp"
		"src/compiler_codegen.cpp"
	INCLUDE_DIRS
		"include"
	REQUIRES
		"TXLib"
)