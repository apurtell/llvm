//===-- llvm-dwarfdump.cpp - Debug info dumping utility for llvm ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This program is a utility that works like "dwarfdump".
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/RelocVisitor.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cstring>
#include <list>
#include <string>
#include <system_error>

using namespace llvm;
using namespace object;
using std::error_code;

static cl::list<std::string>
InputFilenames(cl::Positional, cl::desc("<input object files>"),
               cl::ZeroOrMore);

static cl::opt<DIDumpType>
DumpType("debug-dump", cl::init(DIDT_All),
  cl::desc("Dump of debug sections:"),
  cl::values(
        clEnumValN(DIDT_All, "all", "Dump all debug sections"),
        clEnumValN(DIDT_Abbrev, "abbrev", ".debug_abbrev"),
        clEnumValN(DIDT_AbbrevDwo, "abbrev.dwo", ".debug_abbrev.dwo"),
        clEnumValN(DIDT_Aranges, "aranges", ".debug_aranges"),
        clEnumValN(DIDT_Info, "info", ".debug_info"),
        clEnumValN(DIDT_InfoDwo, "info.dwo", ".debug_info.dwo"),
        clEnumValN(DIDT_Types, "types", ".debug_types"),
        clEnumValN(DIDT_TypesDwo, "types.dwo", ".debug_types.dwo"),
        clEnumValN(DIDT_Line, "line", ".debug_line"),
        clEnumValN(DIDT_LineDwo, "line.dwo", ".debug_line.dwo"),
        clEnumValN(DIDT_Loc, "loc", ".debug_loc"),
        clEnumValN(DIDT_LocDwo, "loc.dwo", ".debug_loc.dwo"),
        clEnumValN(DIDT_Frames, "frames", ".debug_frame"),
        clEnumValN(DIDT_Ranges, "ranges", ".debug_ranges"),
        clEnumValN(DIDT_Pubnames, "pubnames", ".debug_pubnames"),
        clEnumValN(DIDT_Pubtypes, "pubtypes", ".debug_pubtypes"),
        clEnumValN(DIDT_GnuPubnames, "gnu_pubnames", ".debug_gnu_pubnames"),
        clEnumValN(DIDT_GnuPubtypes, "gnu_pubtypes", ".debug_gnu_pubtypes"),
        clEnumValN(DIDT_Str, "str", ".debug_str"),
        clEnumValN(DIDT_StrDwo, "str.dwo", ".debug_str.dwo"),
        clEnumValN(DIDT_StrOffsetsDwo, "str_offsets.dwo", ".debug_str_offsets.dwo"),
        clEnumValEnd));

static void DumpInput(const StringRef &Filename) {
  std::unique_ptr<MemoryBuffer> Buff;

  if (error_code ec = MemoryBuffer::getFileOrSTDIN(Filename, Buff)) {
    errs() << Filename << ": " << ec.message() << "\n";
    return;
  }

  ErrorOr<ObjectFile*> ObjOrErr(ObjectFile::createObjectFile(Buff.release()));
  if (error_code EC = ObjOrErr.getError()) {
    errs() << Filename << ": " << EC.message() << '\n';
    return;
  }
  std::unique_ptr<ObjectFile> Obj(ObjOrErr.get());

  std::unique_ptr<DIContext> DICtx(DIContext::getDWARFContext(Obj.get()));

  outs() << Filename
         << ":\tfile format " << Obj->getFileFormatName() << "\n\n";
  // Dump the complete DWARF structure.
  DICtx->dump(outs(), DumpType);
}

int main(int argc, char **argv) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  cl::ParseCommandLineOptions(argc, argv, "llvm dwarf dumper\n");

  // Defaults to a.out if no filenames specified.
  if (InputFilenames.size() == 0)
    InputFilenames.push_back("a.out");

  std::for_each(InputFilenames.begin(), InputFilenames.end(), DumpInput);

  return 0;
}
