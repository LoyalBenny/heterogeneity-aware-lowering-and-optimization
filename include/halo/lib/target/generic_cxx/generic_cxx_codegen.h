//===- generic_cxx_codegen.h ------------------------------------*- C++ -*-===//
//
// Copyright (C) 2019-2020 Alibaba Group Holding Limited.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#ifndef HALO_LIB_TARGET_GENERIC_CXX_GENERIC_CXX_CODEGEN_H_
#define HALO_LIB_TARGET_GENERIC_CXX_GENERIC_CXX_CODEGEN_H_

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "halo/lib/framework/global_context.h"
#include "halo/lib/ir/common_cast_instructions.h"
#include "halo/lib/ir/common_instructions.h"
#include "halo/lib/ir/common_reduction_instructions.h"
#include "halo/lib/ir/instruction.h"
#include "halo/lib/ir/nn_activation_instructions.h"
#include "halo/lib/ir/nn_cnn_instructions.h"
#include "halo/lib/mm/memory_analyzer.h"
#include "halo/lib/target/codegen.h"

namespace halo {

enum class Dialect {
  CXX_11,
  C99,
};

struct Opts {
  Opts(const bool& en_bf16) : enable_bf16(en_bf16) {}
  Opts() = default;
  bool enable_bf16 = false;
  Dialect dialect = Dialect::CXX_11;
  bool print_mem_stats = false;
  bool emit_value_reset = false;
  bool emit_value_init = false;
  bool emit_value_id_as_int = false;
  CodeGen::ExecMode exec_mode = CodeGen::ExecMode::Compile;
  bool emit_inference_func_sig = false;
  bool emit_dynamic_batch = false;
};

struct CXXType {
  CXXType(const std::string& name)
      : name(name), is_const(false), is_pointer(true) {}
  CXXType() = default;
  std::string Str(bool use_array_decl) const;
  std::string name{"void"};
  bool is_const = false;
  bool is_pointer = true;
};

class CXXValue {
 public:
  CXXValue() : name("undef"), id(-1), type("void"){};
  CXXValue(const std::string& name, const CXXType& type);
  static void Reset();
  std::string name;
  size_t id;
  CXXType type;

 private:
  // make a valid C/C++ identifier name.
  void Normalize(const std::string& n);

  inline static std::unordered_map<std::string, size_t> name2id;
};

// The generic CXX compiler, which is a module pass.
class GenericCXXCodeGen : public CodeGen {
 public:
  GenericCXXCodeGen(std::ostream& os, std::ostream& header_os);
  GenericCXXCodeGen(std::ostream& os, std::ostream& header_os,
                    const Opts& opts);

  virtual ~GenericCXXCodeGen();

  bool RunOnModule(Module* module) override;

 protected:
  virtual void RunOnFunction(Function& function);
  virtual void RunOnHostFunction(Function& function);
  virtual void RunOnConstant(Constant& constant, bool decl);
  virtual void RunOnBasicBlock(BasicBlock& bb);
  void PreRunOnInstruction(Instruction*);
  void PostRunOnInstruction(Instruction*);

  // TODO(unknown): The following RunOnInstruction will be generated via .td
  // file.
  virtual void RunOnInstruction(AddInst*) override;
  virtual void RunOnInstruction(ArgmaxInst*) override;
  virtual void RunOnInstruction(SubInst*) override;
  virtual void RunOnInstruction(MulInst*) override;
  virtual void RunOnInstruction(CallInst*) override;
  virtual void RunOnInstruction(ConcatInst*) override;
  virtual void RunOnInstruction(DivInst*) override;
  virtual void RunOnInstruction(ErfInst*) override;
  virtual void RunOnInstruction(ExpInst*) override;
  virtual void RunOnInstruction(FloorInst*) override;
  virtual void RunOnInstruction(FPtoSIInst*) override;
  virtual void RunOnInstruction(LeakyReluInst*) override;
  virtual void RunOnInstruction(SqrtInst*) override;
  virtual void RunOnInstruction(RsqrtInst*) override;
  virtual void RunOnInstruction(BatchNormInst*) override;
  virtual void RunOnInstruction(BatchMatMulInst*) override;
  virtual void RunOnInstruction(Conv2DInst*) override;
  virtual void RunOnInstruction(Conv2DTransposeInst*) override;
  virtual void RunOnInstruction(GatherInst*) override;
  virtual void RunOnInstruction(GemmInst*) override;
  virtual void RunOnInstruction(LRNInst*) override;
  virtual void RunOnInstruction(MatMulInst*) override;
  virtual void RunOnInstruction(NonMaxSuppressionInst*) override;
  virtual void RunOnInstruction(OneHotInst*) override;
  virtual void RunOnInstruction(PadInst*) override;
  virtual void RunOnInstruction(PoolingMaxInst*) override;
  virtual void RunOnInstruction(PoolingAvgInst*) override;
  virtual void RunOnInstruction(ReduceMeanInst*) override;
  virtual void RunOnInstruction(ReluInst*) override;
  virtual void RunOnInstruction(Relu6Inst*) override;
  virtual void RunOnInstruction(ReshapeInst*) override;
  virtual void RunOnInstruction(ResizeInst*) override;
  virtual void RunOnInstruction(ReturnInst*) override;
  virtual void RunOnInstruction(SliceInst*) override;
  virtual void RunOnInstruction(SoftmaxInst*) override;
  virtual void RunOnInstruction(SigmoidInst*) override;
  virtual void RunOnInstruction(TopKInst*) override;
  virtual void RunOnInstruction(TransposeInst*) override;
  virtual void RunOnBinaryInstruction(Instruction*);
  virtual void RunOnUnaryInstruction(Instruction*);

  virtual CXXValue AllocateBuffer(const Def& def, bool on_stack);
  std::string GetFunctionDecl(const Function& func, const Instruction& ret_inst,
                              bool with_func_name, bool with_type);
  virtual std::string EmitShape(const halo::Type& type);
  virtual std::string EmitType(const halo::Type& type);
  virtual std::string EmitLValue(const std::string& name) const;

  void EmitODLAArgs(const std::vector<int32_t>& arg);
  void EmitODLAArgs(const std::vector<uint32_t>& arg);
  void EmitODLAArgs(const std::vector<CXXValue>& arg);
  void EmitODLAArgs(const halo::Type& arg);
  void EmitODLAArgs(const DataType& arg);
  void EmitODLAArgs(const CXXValue& arg);
  void EmitODLAArgs(const bool& arg);
  void EmitODLAArgs(const DataFormat& arg);

  template <typename T>
  void EmitODLAArgs(const T& arg) {
    os_ << arg;
  }

  template <typename T, typename... Targs>
  void EmitODLAArgs(T arg, Targs... args) {
    EmitODLAArgs(arg);
    os_ << ", ";
    EmitODLAArgs(args...);
  }

  template <int indent = 2, bool is_op = true, typename... Targs>
  void EmitODLACall(const CXXValue& lhs, const char* func_name, Targs... args) {
    os_ << std::string(indent, ' ');
    os_ << EmitLValue(lhs.name) << " = ";
    os_ << func_name << "(";
    EmitODLAArgs(args...);
    if (is_op) {
      os_ << ", (const odla_value_id)";
      if (opts_.emit_value_id_as_int) {
        os_ << lhs.id;
      } else {
        os_ << "\"" << lhs.name << "\"";
      }
    }
    os_ << ");\n";
  }

  virtual const std::string& EmitNull() const noexcept;
  virtual std::string GetODLAType(halo::DataType data_type) const noexcept;
  static const std::string& EmitReturnType(bool auto_type);
  static CXXType SNTypeToCXXType(DataType dt);
  static CXXType TensorTypeToCXXType(const halo::Type& type, bool is_const);

  template <typename T>
  inline static std::string Join(std::vector<T> vals, char sep = ',') {
    std::ostringstream ss;
    bool is_first = true;
    for (const auto& val : vals) {
      if (!is_first) {
        ss << sep << " ";
      }
      ss << val;
      is_first = false;
    }
    return ss.str();
  }
  template <typename T>
  inline static std::string Join(T value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
  }
  template <typename T, typename... Targs>
  inline static std::string Join(T value, Targs... args) {
    std::ostringstream ss;
    ss << value << ", ";
    ss << Join(args...);
    return ss.str();
  }

  std::ostream& os_;
  std::ostream& header_os_;
  GlobalContext* ctx_ = nullptr;
  std::unordered_map<Def, CXXValue> ir_mapping_;
  std::unique_ptr<MemoryAnalyzer> memory_analyzer_;
  Opts opts_;
};

class GenericCXXConstantWriter : public GenericCXXCodeGen {
 public:
  virtual ~GenericCXXConstantWriter() = default;
  explicit GenericCXXConstantWriter(std::ostream& os);

  bool RunOnModule(Module* module) override;

 private:
  void static RunOnConstant(const Constant& constant, std::ostream* os);
};

} // end namespace halo.

#endif // HALO_LIB_TARGET_GENERIC_CXX_GENERIC_CXX_CODEGEN_H_
