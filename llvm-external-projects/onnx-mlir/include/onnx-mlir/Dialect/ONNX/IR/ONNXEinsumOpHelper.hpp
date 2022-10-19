/*
 * SPDX-License-Identifier: Apache-2.0
 */

//===-------- ONNXEinsumOpHelper.hpp - Helper functions for Einsum --------===//
//
// This file contains helper functions for processing the ONNX Einsum Operator.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "onnx-mlir/Dialect/ONNX/IR/ONNXOps.hpp"

#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <stdint.h>

namespace mlir {
class InFlightDiagnostic;
} // namespace mlir

namespace mlir::onnx_mlir {

namespace einsum {

typedef llvm::function_ref<mlir::InFlightDiagnostic()> ErrorFn;

mlir::LogicalResult verifyEquation(
    llvm::StringRef equation, size_t numInputs, ErrorFn emitErrorFn);

mlir::LogicalResult verifyShapes(
    ONNXEinsumOpAdaptor operandAdaptor, ErrorFn emitErrorFn);

typedef llvm::SmallVector<int64_t, 4> Shape;

mlir::FailureOr<Shape> inferOutputShape(
    ONNXEinsumOpAdaptor operandAdaptor, ErrorFn emitErrorFn);

typedef llvm::SmallString<4> Subscripts;

struct Parameter {
  Shape shape;
  Subscripts subscripts;
};

struct Signature {
  llvm::SmallVector<Parameter> inputs;
  Parameter output;
};

mlir::FailureOr<Signature> inferSignature(
    ONNXEinsumOpAdaptor operandAdaptor, ErrorFn emitErrorFn);

} // namespace einsum

} // namespace mlir::onnx_mlir
