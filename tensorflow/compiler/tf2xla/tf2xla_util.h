/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_TF2XLA_TF2XLA_UTIL_H_
#define TENSORFLOW_COMPILER_TF2XLA_TF2XLA_UTIL_H_

#include <unordered_map>

#include "absl/strings/string_view.h"
#include "tensorflow/compiler/tf2xla/tf2xla.pb.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/kernel_def.pb.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/lib/core/status.h"

namespace tensorflow {

// ValidateConfig returns OK iff config is valid.
Status ValidateConfig(const tf2xla::Config& config);

// Modifies <graph_def> to include placeholders for each fed tensor, and
// update references to the fed tensors to refer to the placeholders.
// The existing nodes referenced by the feeds are not removed or modified
// (except where their input edges are modified by the replacement of other
// feeds).
Status AddPlaceholdersForFeeds(
    const tf2xla::Config& config, const OpRegistryInterface* op_registry,
    std::unordered_map<string, string>* feed_remapping, GraphDef* graph_def);

// Returns in <out> a copy of <in>, pruned to only include fetches from
// <config>.
Status PruneGraphDefInto(const tf2xla::Config& config, const GraphDef& in,
                         GraphDef* out);

// Returns node:port for the given <id>.
string TensorIdToString(const tf2xla::TensorId& id);

// Updates the sharding of <n> based on the sharding of its neighbors.
// If <out_edges> is true, outgoing edges from <n> are considered; else incoming
// edges are considered.
Status SetNodeShardingFromNeighbors(Node* n, bool out_edges);

// Add an allowed data type to the AttrConstraint with the given name.
void AddDtypeToKernalDefConstraint(absl::string_view name, DataType dtype,
                                   KernelDef* kdef);

// Returns the next random seed to use for seeding xla rng.
uint32 GetXLARandomSeed();

// Indicates how a FunctionDef is associated with a graph node (e.g. the node is
// a function call, or the node has function attrs).
class AssociatedFunctionInfo {
 public:
  enum AssociatedFunctionType {
    kFunctionCallNode = 0,
    kFunctionAttr = 1,
  };

  // The node is a function call.
  AssociatedFunctionInfo(const string& func_name, const AttrValueMap& attrs)
      : type_(kFunctionCallNode), func_name_(func_name), attrs_(attrs) {}

  // The function is an attr of the node.
  AssociatedFunctionInfo(const string& func_name, const AttrValueMap& attrs,
                         const string& attr_name)
      : type_(kFunctionAttr),
        func_name_(func_name),
        attrs_(attrs),
        attr_name_(attr_name) {}

  AssociatedFunctionType type() const { return type_; }

  const string& func_name() const { return func_name_; }

  const string& attr_name() const { return attr_name_; }

  const AttrValueMap& attrs() const { return attrs_; }

 private:
  // Available for all instances.
  AssociatedFunctionType type_;
  string func_name_;
  AttrValueMap attrs_;

  // Only available if the function is defined in an attr.
  string attr_name_;
};

// Returns if the NodeDef has associated function.
bool HasAssociatedFunction(const NodeDef& node_def,
                           FunctionLibraryRuntime* flr);

// Gets functions associated with the node. Current cases:
// 1. For function call node, its function name;
// 2. For nodes like XlaWhile/XlaIf, all their function attributes.
std::vector<AssociatedFunctionInfo> GetAssociatedFunctions(
    const Node& node, FunctionLibraryRuntime* flr);

// Changes associated functions for the node. Current cases:
// 1. For function call node, creates a new node with the new function name and
//    remove the old node;
// 2. For nodes like XlaWhile/XlaIf, modify their function attributes.
Status RewriteAssociatedFunction(
    Graph* graph, Node* node, FunctionLibraryDefinition* fld,
    const AssociatedFunctionInfo& associated_function,
    const string& rewritten_function_name);

// Attribute to mark nodes to be executed on host.
extern const char kXlaOutsideCompilationAttrName[];

}  // namespace tensorflow

#endif  // TENSORFLOW_COMPILER_TF2XLA_TF2XLA_UTIL_H_
