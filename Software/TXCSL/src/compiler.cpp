// Copyright (c) 2026 TXCSL. Licensed under the MIT License.

#include "impl/basic_utils.hpp"
#include "impl/csl/definition.hpp"
#include "impl/csl/compiler.hpp"
#include "impl/csl/compiler_codegen.hpp"

namespace tx::csl {
CompileResult Compiler::compile(std::string_view source, Expression& result) {
	CodeGen_impl compiler(source, result);
	return compiler.run();
}
} // namespace tx::csl