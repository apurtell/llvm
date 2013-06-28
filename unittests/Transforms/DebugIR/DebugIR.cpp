//===- DebugIR.cpp - Unit tests for the DebugIR pass ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The tests in this file verify the DebugIR pass that generates debug metadata
// for LLVM IR.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Triple.h"
#include "llvm/Config/config.h"
#include "llvm/DebugInfo.h"
#include "llvm/DIBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Transforms/Instrumentation.h"

#include "../lib/Transforms/Instrumentation/DebugIR.h"

// These tests do not depend on MCJIT, but we use the TrivialModuleBuilder
// helper class to construct some trivial Modules.
#include "../unittests/ExecutionEngine/MCJIT/MCJITTestBase.h"

#include <string>

#include "gtest/gtest.h"

#if defined(LLVM_ON_WIN32)
#include <direct.h>
#define getcwd_impl _getcwd
#elif defined (HAVE_GETCWD)
#include <unistd.h>
#define getcwd_impl getcwd
#endif // LLVM_ON_WIN32

using namespace llvm;
using namespace std;

namespace {

/// Insert a mock CUDescriptor with the specified producer
void insertCUDescriptor(Module *M, StringRef File, StringRef Dir,
                        StringRef Producer) {
  DIBuilder B(*M);
  B.createCompileUnit(dwarf::DW_LANG_C99, File, Dir, Producer, false, "", 0);
  B.finalize();
}

/// Attempts to remove file at Path and returns true if it existed, or false if
/// it did not.
bool removeIfExists(StringRef Path) {
  bool existed = false;
  sys::fs::remove(Path, existed);
  return existed;
}

char * current_dir() {
#if defined(LLVM_ON_WIN32) || defined(HAVE_GETCWD)
  // calling getcwd (or _getcwd() on windows) with a null buffer makes it
  // allocate a sufficiently sized buffer to store the current working dir.
  return getcwd_impl(0, 0);
#else
  return 0;
#endif
}

class TestDebugIR : public ::testing::Test, public TrivialModuleBuilder {
protected:
  TestDebugIR()
      : TrivialModuleBuilder(sys::getProcessTriple())
      , cwd(current_dir()) {}

  ~TestDebugIR() { free(cwd); }

  /// Returns a concatenated path string consisting of Dir and Filename
  string getPath(const string &Dir, const string &Filename) {
    SmallVector<char, 8> Path;
    sys::path::append(Path, Dir, Filename);
    Path.resize(Dir.size() + Filename.size() + 2);
    Path[Dir.size() + Filename.size() + 1] = '\0';
    return string(Path.data());
  }

  LLVMContext Context;
  char *cwd;
  OwningPtr<Module> M;
  OwningPtr<DebugIR> D;
};

// Test empty named Module that is not supposed to be output to disk.
TEST_F(TestDebugIR, EmptyNamedModuleNoWrite) {
  string name = "/mock/path/to/empty_module.ll";
  M.reset(createEmptyModule(name));
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass()));
  string Path;
  D->runOnModule(*M, Path);

  // verify DebugIR was able to correctly parse the file name from module ID
  ASSERT_EQ(Path, name);

  // verify DebugIR did not generate a file
  ASSERT_FALSE(removeIfExists(Path)) << "Unexpected file " << Path;
}

// Test a non-empty unnamed module that is output to an autogenerated file name.
TEST_F(TestDebugIR, NonEmptyUnnamedModuleWriteToAutogeneratedFile) {
  M.reset(createEmptyModule());
  insertAddFunction(M.get());
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(true, true)));

  string Path;
  D->runOnModule(*M, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

// Test not specifying a name in the module -- DebugIR should generate a name
// and write the file contents.
TEST_F(TestDebugIR, EmptyModuleWriteAnonymousFile) {
  M.reset(createEmptyModule());
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(false, false)));

  string Path;
  D->runOnModule(*M, Path);

  // verify DebugIR generated a file and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

#ifdef HAVE_GETCWD // These tests require get_current_dir_name()

// Test empty named Module that is to be output to path specified at Module
// construction.
TEST_F(TestDebugIR, EmptyNamedModuleWriteFile) {
  string Filename("NamedFile1");
  string ExpectedPath(getPath(cwd, Filename));

  M.reset(createEmptyModule(ExpectedPath));
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(true, true)));

  string Path;
  D->runOnModule(*M, Path);

  // verify DebugIR was able to correctly parse the file name from module ID
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

// Test an empty unnamed module generates an output file whose path is specified
// at DebugIR construction.
TEST_F(TestDebugIR, EmptyUnnamedModuleWriteNamedFile) {
  string Filename("NamedFile2");

  M.reset(createEmptyModule());
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(
      false, false, StringRef(cwd), StringRef(Filename))));
  string Path;
  D->runOnModule(*M, Path);

  string ExpectedPath(getPath(cwd, Filename));
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

// Test an empty named module generates an output file at the path specified
// during DebugIR construction.
TEST_F(TestDebugIR, EmptyNamedModuleWriteNamedFile) {
  string Filename("NamedFile3");

  string UnexpectedPath(getPath(cwd, "UnexpectedFilename"));
  M.reset(createEmptyModule(UnexpectedPath));

  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(
      false, false, StringRef(cwd), StringRef(Filename))));
  string Path;
  D->runOnModule(*M, Path);

  string ExpectedPath(getPath(cwd, Filename));
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;

  // verify DebugIR did not generate a file at the path specified at Module
  // construction.
  ASSERT_FALSE(removeIfExists(UnexpectedPath)) << "Unexpected file " << Path;
}

// Test a non-empty named module that is not supposed to be output to disk
TEST_F(TestDebugIR, NonEmptyNamedModuleNoWrite) {
  string Filename("NamedFile4");
  string ExpectedPath(getPath(cwd, Filename));

  M.reset(createEmptyModule(ExpectedPath));
  insertAddFunction(M.get());

  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass()));

  string Path;
  D->runOnModule(*M, Path);
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR did not generate a file
  ASSERT_FALSE(removeIfExists(Path)) << "Unexpected file " << Path;
}

// Test a non-empty named module that is output to disk.
TEST_F(TestDebugIR, NonEmptyNamedModuleWriteFile) {
  string Filename("NamedFile5");
  string ExpectedPath(getPath(cwd, Filename));

  M.reset(createEmptyModule(ExpectedPath));
  insertAddFunction(M.get());

  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass(true, true)));

  string Path;
  D->runOnModule(*M, Path);
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

// Test a non-empty unnamed module is output to a path specified at DebugIR
// construction.
TEST_F(TestDebugIR, NonEmptyUnnamedModuleWriteToNamedFile) {
  string Filename("NamedFile6");

  M.reset(createEmptyModule());
  insertAddFunction(M.get());

  D.reset(static_cast<DebugIR *>(
      llvm::createDebugIRPass(true, true, cwd, Filename)));
  string Path;
  D->runOnModule(*M, Path);

  string ExpectedPath(getPath(cwd, Filename));
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR generated a file, and clean it up
  ASSERT_TRUE(removeIfExists(Path)) << "Missing expected file at " << Path;
}

// Test that information inside existing debug metadata is retained
TEST_F(TestDebugIR, ExistingMetadataRetained) {
  string Filename("NamedFile7");
  string ExpectedPath(getPath(cwd, Filename));

  M.reset(createEmptyModule(ExpectedPath));
  insertAddFunction(M.get());

  StringRef Producer("TestProducer");
  insertCUDescriptor(M.get(), Filename, cwd, Producer);

  DebugInfoFinder Finder;
  Finder.processModule(*M);
  ASSERT_EQ((unsigned)1, Finder.compile_unit_count());
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass()));

  string Path;
  D->runOnModule(*M, Path);
  ASSERT_EQ(ExpectedPath, Path);

  // verify DebugIR did not generate a file
  ASSERT_FALSE(removeIfExists(Path)) << "Unexpected file " << Path;

  DICompileUnit CU(*Finder.compile_unit_begin());

  // Verify original CU information is retained
  ASSERT_EQ(Filename, CU.getFilename());
  ASSERT_EQ(cwd, CU.getDirectory());
  ASSERT_EQ(Producer, CU.getProducer());
}

#endif // HAVE_GETCWD

#ifdef GTEST_HAS_DEATH_TEST

// Test a non-empty unnamed module that is not supposed to be output to disk
// NOTE: this test is expected to die with LLVM_ERROR, and such depends on
// google test's "death test" mode.
TEST_F(TestDebugIR, NonEmptyUnnamedModuleNoWrite) {
  M.reset(createEmptyModule(StringRef()));
  insertAddFunction(M.get());
  D.reset(static_cast<DebugIR *>(llvm::createDebugIRPass()));

  // No name in module or on DebugIR construction ==> DebugIR should assert
  EXPECT_DEATH(D->runOnModule(*M),
               "DebugIR unable to determine file name in input.");
}

#endif // GTEST_HAS_DEATH_TEST
}
