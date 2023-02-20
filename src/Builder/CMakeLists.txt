# SPDX-License-Identifier: Apache-2.0

# If building as an LLVM external project, don't use LLVM's compile flags for this
# library. The reason is that it must be compiled with exceptions on, while the larger
# build may compile LLVM without exceptions or RTTI
set(other_args_)
if(ONNX_MLIR_EXTERNAL_PROJECT_BUILD)
  set(other_args_ NO_LLVM_FLAGS)
endif()

# This will cause onnx to be built. More importantly, some variable definitions
# when building onnx such as -DONNX_ML=1 -DONNX_NAMESPACE=onnx will be carried over
# when compiling FrontendDialectHelper.cpp, etc. because they are properly exported
# as part of the onnx target and target_link_libraries interprets them correctly.
add_onnx_mlir_library(OMBuilder
  FrontendDialectHelper.cpp
  FrontendDialectTransformer.cpp
  ImportONNXUtils.cpp
  ModelInputShaper.cpp

  LINK_LIBS PUBLIC
  OMHasOnnxSubgraphOpInterface
  OMONNXOps
  OMResultTypeInferenceOpInterface
  MLIRFuncDialect
  onnx

  ${other_args_}
  )

configure_file(OpBuildTable.inc.dc.in
  ${CMAKE_CURRENT_BINARY_DIR}/OpBuildTable.inc.dc
  @ONLY
  )
