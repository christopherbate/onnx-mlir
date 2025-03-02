/*
 * SPDX-License-Identifier: Apache-2.0
 */

//===---------------- ONNXShapeHelper.hpp - help for shapes ---------------===//
//
// Copyright 2022 The IBM Research Authors.
//
// =============================================================================
//
// This file has the computations to compute the shapes using the new index expr
// approach.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <utility>

#include "llvm/ADT/SmallVector.h"

#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

#include "src/Dialect/Mlir/DialectBuilder.hpp"
#include "src/Dialect/Mlir/IndexExpr.hpp"

namespace onnx_mlir {

// ===----------------------------------------------------------------------===//
//  IndexShapeBuilder
// ===----------------------------------------------------------------------===/

/*
  IndexExprBuilder is used to extract index expressions for computations
  typically related to shapes. This class defines all the algorithms but rely
  on subclass to extract "runtime" values. Methods are provided to return
  literal/symbol/dim index expressions related to operation attributes,
  operation operands, and the shape of operands.

  Recall that literals are compile-time integer values, and symbol and dim are
  runtime values. The difference between symbol/dim related to affine
  expression; symbol is not changing in the given context (e.g. batch size in a
  given loop), and dim are changing (e.g. the loop index inside a given loop).

  This class cannot be directly used, as subclasses must redefine 3 pure virtual
  functions (getConst, getVal, and getShape) to provide the proper values for
  the methods defined in this class.

  A first subclass is IndexExprBuilderForAnalysis and is used during the
  analysis phase; runtime values are described by questionmark index
  expressions.

  Other subclasses (e.g. IndexExprBuilderForKrnl/IndexExprBuilderForMhlo )
  generate dialect operations (e.g. Krnl/Mhlo ops) to generate code that compute
  runtime values.
*/

/* Dialect use:
   May generate math conversion ops, plus  what is possibly generated in the
   virtual subclass method implementation for getConst, getVal, getShapeVal.
*/

struct IndexExprBuilder : DialectBuilder {
  // Constructor for analysis (no code generation, will assert if it tries).
  IndexExprBuilder(mlir::Location loc) : DialectBuilder(loc) {}
  // Constructors for code generation.
  IndexExprBuilder(mlir::OpBuilder &b, mlir::Location loc)
      : DialectBuilder(b, loc) {}
  IndexExprBuilder(const DialectBuilder &db) : DialectBuilder(db) {}
  virtual ~IndexExprBuilder() {}

  using IndexExprList = llvm::SmallVectorImpl<IndexExpr>;

  //===--------------------------------------------------------------------===//
  // Get info about rank and sizes

  // check/assert that value has shape and rank.
  bool hasShapeAndRank(mlir::Value value);
  void assertHasShapeAndRank(mlir::Value value);

  // Get rank of the type defined by value. Expect ranked shaped type.
  uint64_t getShapedTypeRank(mlir::Value value);
  // Get size of 1D array attribute. Expect 1D ranked shaped type.
  uint64_t getArraySize(mlir::ArrayAttr arrayAttr);
  // Get size of 1D array defined by arrayVal. Expect 1D ranked shaped type.
  uint64_t getArraySize(mlir::Value arrayVal);

  //===--------------------------------------------------------------------===//
  // Get literal index expressions from an array of integer attributes.
  // Typically used for getting literals out of operation's integer attributes.
  // There is no support for ranks higher than 1 at this time.

  // Get literal index expression from the value of an array attribute at
  // position i. When out of bound, return an undefined index expression.
  IndexExpr getIntFromArrayAsLiteral(mlir::ArrayAttr intAttrArray, uint64_t i);
  // Same as above; `outOfBoundVal` literal index expression is returned when
  // out of bound.
  IndexExpr getIntFromArrayAsLiteral(
      mlir::ArrayAttr intAttrArray, uint64_t i, int64_t outOfBoundVal);
  // Same as above, but get a list of up to len values. A length of -1 returns
  // the whole list. Assert when `len` exceed the array bounds.
  void getIntFromArrayAsLiterals(
      mlir::ArrayAttr intAttrArray, IndexExprList &list, int64_t len = -1);

  //===--------------------------------------------------------------------===//
  // Get symbol/dim index expressions from a scalar or 1D array value. When
  // the values are defined by a constant, then literal index expressions are
  // return in place of a symbol index expression. With dynamic values,
  // questionmark index expressions are returned during code analysis phases and
  // symbol index expressions are returned during code generation phases. Note
  // that array of rank 0 are treated as scalars. Introduce conversions to index
  // type when input is in a different type.
  //
  // There is no support for ranks higher than 1 at this time.  Expects a shaped
  // type with a known rank.

  // Get a symbol/dim index expression defined by `value`.
  IndexExpr getIntAsSymbol(mlir::Value value);
  IndexExpr getIntAsDim(mlir::Value value);
  // Get a symbol/dim index expression from the array defined by `array` at
  // position `i`. When out of bound, return an undefined index expressions.
  IndexExpr getIntFromArrayAsSymbol(mlir::Value array, uint64_t i);
  IndexExpr getIntFromArrayAsDim(mlir::Value array, uint64_t i);
  // Same as above; `outOfBoundVal` literal index expression is returned when
  // out of bound.
  IndexExpr getIntFromArrayAsSymbol(
      mlir::Value array, uint64_t i, int64_t outOfBoundVal);
  IndexExpr getIntFromArrayAsDim(
      mlir::Value array, uint64_t i, int64_t outOfBoundVal);
  // Same as above, but get a list of up to len values. A length of -1 returns
  // the whole list. Assert when `len` exceed the array bounds.
  void getIntFromArrayAsSymbols(
      mlir::Value intArrayVal, IndexExprList &list, int64_t len = -1);
  void getIntFromArrayAsDims(
      mlir::Value intArrayVal, IndexExprList &list, int64_t len = -1);

  //===--------------------------------------------------------------------===//
  // Get info from tensor/memref shape. Return literal index expressions when a
  // shape is known at compile time. Returns a questionmark for a runtime shape
  // during analysis phases. Returns a symbol or dim index expression for a
  // runtime shape during code generation phases. Works on tensors and memrefs.
  // Asserts when requesting out of bound shapes.  Expects a shaped type with a
  // known rank.

  // Return true if shape is known at compile time, i.e. is a literal value.
  bool isLiteralShape(mlir::Value tensorOrMemrefValue, uint64_t i);
  // Return true if the entire shape is known at compile time.
  bool isLiteralShape(mlir::Value tensorOrMemrefValue);
  // Get the raw shape (as integer, -1 when runtime value).
  int64_t getShape(mlir::Value tensorOrMemrefValue, uint64_t i);

  // Get an index expression from tensor/memref shape.
  IndexExpr getShapeAsLiteral(mlir::Value tensorOrMemrefValue, uint64_t i);
  IndexExpr getShapeAsSymbol(mlir::Value tensorOrMemrefValue, uint64_t i);
  IndexExpr getShapeAsDim(mlir::Value tensorOrMemrefValue, uint64_t i);
  // Get an index expression list from tensor/memref shape.
  void getShapeAsLiterals(mlir::Value tensorOrMemrefValue, IndexExprList &list);
  void getShapeAsSymbols(mlir::Value tensorOrMemrefValue, IndexExprList &list);
  void getShapeAsDims(mlir::Value tensorOrMemrefValue, IndexExprList &list);

protected:
  //===--------------------------------------------------------------------===//
  // Subclasses must define these pure virtual functions.

  // Locate a dense element attribute associated with the defining op given by
  // value. Return nullptr if none exists.
  virtual mlir::DenseElementsAttr getConst(mlir::Value value) = 0;
  // Locate/generate a value that represents a value given by the op defining
  // arrayVal at position i in the array. Return nullptr if cannot
  // locate/generate the value.
  virtual mlir::Value getVal(mlir::Value arrayVal, uint64_t i) = 0;
  // Locate/generate a value that represents the integer value of the shape
  // given by a tensor or memref at position i. Return nullptr if cannot
  // locate/generate the value.
  virtual mlir::Value getShapeVal(
      mlir::Value tensorOrMemrefValue, uint64_t i) = 0;

private:
  // Returns a SymbolIndexExpr/DimIndexExpr when makeSymbol is true/false.
  IndexExpr getIntFromArray(mlir::Value array, uint64_t i, bool makeSymbol);
};

} // namespace onnx_mlir
