//===- yaml2obj - Convert YAML to a binary object file --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This program takes a YAML description of an object file and outputs the
// binary equivalent.
//
// This is used for writing tests that require binary files.
//
//===----------------------------------------------------------------------===//

#include "llvm/ObjectYAML/yaml2obj.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ObjectYAML/ObjectYAML.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"
#include <system_error>

using namespace llvm;

static cl::opt<std::string>
  Input(cl::Positional, cl::desc("<input>"), cl::init("-"));

cl::opt<unsigned>
DocNum("docnum", cl::init(1),
       cl::desc("Read specified document from input (default = 1)"));

static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"));

LLVM_ATTRIBUTE_NORETURN static void error(Twine Message) {
  errs() << Message << "\n";
  exit(1);
}

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv);

  if (OutputFilename.empty())
    OutputFilename = "-";

  std::error_code EC;
  std::unique_ptr<ToolOutputFile> Out(
      new ToolOutputFile(OutputFilename, EC, sys::fs::OF_None));
  if (EC)
    error("yaml2obj: Error opening '" + OutputFilename + "': " + EC.message());

  ErrorOr<std::unique_ptr<MemoryBuffer>> Buf =
      MemoryBuffer::getFileOrSTDIN(Input);
  if (!Buf)
    return 1;

  yaml::Input YIn(Buf.get()->getBuffer());
  if (Error E = convertYAML(YIn, Out->os(), DocNum)) {
    logAllUnhandledErrors(std::move(E), WithColor::error(errs(), argv[0]));
    return 1;
  }

  Out->keep();
  Out->os().flush();
  return 0;
}
