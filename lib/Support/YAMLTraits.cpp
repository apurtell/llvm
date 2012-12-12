//===- lib/Support/YAMLTraits.cpp -----------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define BUILDING_YAMLIO
#include "llvm/Support/YAMLTraits.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/raw_ostream.h"
#include <cstring>

namespace llvm {
namespace yaml {



//===----------------------------------------------------------------------===//
//  IO
//===----------------------------------------------------------------------===//

IO::IO(void *Context) : Ctxt(Context) {
}

IO::~IO() {
}

void *IO::getContext() {
  return Ctxt;
}

void IO::setContext(void *Context) {
  Ctxt = Context;
}


//===----------------------------------------------------------------------===//
//  Input
//===----------------------------------------------------------------------===//

Input::Input(StringRef InputContent, void *Ctxt)
    : IO(Ctxt), CurrentNode(NULL) {
  Strm = new Stream(InputContent, SrcMgr);
  DocIterator = Strm->begin();
}


llvm::error_code Input::error() {
  return EC;
}

void Input::setDiagHandler(llvm::SourceMgr::DiagHandlerTy Handler, void *Ctxt) {
  SrcMgr.setDiagHandler(Handler, Ctxt);
}

bool Input::outputting() {
  return false;
}

bool Input::setCurrentDocument() {
  if ( DocIterator != Strm->end() ) {
    Node *N = DocIterator->getRoot();
    if (llvm::isa<NullNode>(N)) {
      // Empty files are allowed and ignored
      ++DocIterator;
      return setCurrentDocument();
    }
    CurrentNode = this->createHNodes(N);
    return true;
  }
  return false;
}

void Input::nextDocument() {
  ++DocIterator;
}

void Input::beginMapping() {
  if ( EC )
    return;
  MapHNode *MN = llvm::dyn_cast<MapHNode>(CurrentNode);
  if ( MN ) {
    MN->ValidKeys.clear();
  }
}

bool Input::preflightKey(const char *Key, bool Required, bool,
                                          bool &UseDefault, void *&SaveInfo) {
  UseDefault = false;
  if ( EC )
    return false;
  MapHNode *MN = llvm::dyn_cast<MapHNode>(CurrentNode);
  if ( !MN ) {
    setError(CurrentNode, "not a mapping");
    return false;
  }
  MN->ValidKeys.push_back(Key);
  HNode *Value = MN->Mapping[Key];
  if ( !Value ) {
    if ( Required )
      setError(CurrentNode, Twine("missing required key '") + Key + "'");
    else
      UseDefault = true;
   return false;
  }
  SaveInfo = CurrentNode;
  CurrentNode = Value;
  return true;
}

void Input::postflightKey(void *saveInfo) {
  CurrentNode = reinterpret_cast<HNode*>(saveInfo);
}

void Input::endMapping() {
  if ( EC )
    return;
  MapHNode *MN = llvm::dyn_cast<MapHNode>(CurrentNode);
  if ( !MN )
    return;
  for (MapHNode::NameToNode::iterator i=MN->Mapping.begin(),
                                        End=MN->Mapping.end(); i != End; ++i) {
    if ( ! MN->isValidKey(i->first) ) {
       setError(i->second, Twine("unknown key '") + i->first + "'" );
      break;
    }
  }
}


unsigned Input::beginSequence() {
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    return SQ->Entries.size();
  }
  return 0;
}
void Input::endSequence() {
}
bool Input::preflightElement(unsigned Index, void *&SaveInfo) {
  if ( EC )
    return false;
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    SaveInfo = CurrentNode;
    CurrentNode = SQ->Entries[Index];
    return true;
  }
  return false;
}
void Input::postflightElement(void *SaveInfo) {
  CurrentNode = reinterpret_cast<HNode*>(SaveInfo);
}

unsigned Input::beginFlowSequence() {
   if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    return SQ->Entries.size();
  }
  return 0;
}
bool Input::preflightFlowElement(unsigned index, void *&SaveInfo) {
  if ( EC )
    return false;
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    SaveInfo = CurrentNode;
    CurrentNode = SQ->Entries[index];
    return true;
  }
  return false;
}
void Input::postflightFlowElement(void *SaveInfo) {
  CurrentNode = reinterpret_cast<HNode*>(SaveInfo);
}
void Input::endFlowSequence() {
}


void Input::beginEnumScalar() {
  ScalarMatchFound = false;
}

bool Input::matchEnumScalar(const char *Str, bool) {
  if ( ScalarMatchFound )
    return false;
  if ( ScalarHNode *SN = llvm::dyn_cast<ScalarHNode>(CurrentNode) ) {
    if ( SN->value().equals(Str) ) {
      ScalarMatchFound = true;
      return true;
    }
  }
  return false;
}

void Input::endEnumScalar() {
  if ( !ScalarMatchFound ) {
    setError(CurrentNode, "unknown enumerated scalar");
  }
}



bool Input::beginBitSetScalar(bool &DoClear) {
  BitValuesUsed.clear();
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    BitValuesUsed.insert(BitValuesUsed.begin(), SQ->Entries.size(), false);
  }
  else {
    setError(CurrentNode, "expected sequence of bit values");
  }
  DoClear = true;
  return true;
}

bool Input::bitSetMatch(const char *Str, bool) {
  if ( EC )
    return false;
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    unsigned Index = 0;
    for (std::vector<HNode*>::iterator i=SQ->Entries.begin(),
                                       End=SQ->Entries.end(); i != End; ++i) {
      if ( ScalarHNode *SN = llvm::dyn_cast<ScalarHNode>(*i) ) {
        if ( SN->value().equals(Str) ) {
          BitValuesUsed[Index] = true;
          return true;
        }
      }
      else {
        setError(CurrentNode, "unexpected scalar in sequence of bit values");
      }
      ++Index;
    }
  }
  else {
    setError(CurrentNode, "expected sequence of bit values");
  }
  return false;
}

void Input::endBitSetScalar() {
  if ( EC )
    return;
  if ( SequenceHNode *SQ = llvm::dyn_cast<SequenceHNode>(CurrentNode) ) {
    assert(BitValuesUsed.size() == SQ->Entries.size());
    for ( unsigned i=0; i < SQ->Entries.size(); ++i ) {
      if ( !BitValuesUsed[i] ) {
        setError(SQ->Entries[i], "unknown bit value");
        return;
      }
    }
  }
}


void Input::scalarString(StringRef &S) {
  if ( ScalarHNode *SN = llvm::dyn_cast<ScalarHNode>(CurrentNode) ) {
    S = SN->value();
  }
  else {
    setError(CurrentNode, "unexpected scalar");
  }
}

void Input::setError(HNode *hnode, const Twine &message) {
  this->setError(hnode->_node, message);
}

void Input::setError(Node *node, const Twine &message) {
  Strm->printError(node, message);
  EC = make_error_code(errc::invalid_argument);
}

Input::HNode *Input::createHNodes(Node *N) {
  llvm::SmallString<128> StringStorage;
  if ( ScalarNode *SN = llvm::dyn_cast<ScalarNode>(N) ) {
    StringRef KeyStr = SN->getValue(StringStorage);
    if ( !StringStorage.empty() ) {
      // Copy string to permanent storage
      unsigned Len = StringStorage.size();
      char* Buf = Allocator.Allocate<char>(Len);
      memcpy(Buf, &StringStorage[0], Len);
      KeyStr = StringRef(Buf, Len);
    }
    return new (Allocator) ScalarHNode(N, KeyStr);
  }
  else if ( SequenceNode *SQ = llvm::dyn_cast<SequenceNode>(N) ) {
    SequenceHNode *SQHNode = new (Allocator) SequenceHNode(N);
    for (SequenceNode::iterator i=SQ->begin(),End=SQ->end(); i != End; ++i ) {
      HNode *Entry = this->createHNodes(i);
      if ( EC )
        break;
      SQHNode->Entries.push_back(Entry);
    }
    return SQHNode;
  }
  else if ( MappingNode *Map = llvm::dyn_cast<MappingNode>(N) ) {
    MapHNode *mapHNode = new (Allocator) MapHNode(N);
    for (MappingNode::iterator i=Map->begin(), End=Map->end(); i != End; ++i ) {
      ScalarNode *KeyScalar = llvm::dyn_cast<ScalarNode>(i->getKey());
      StringStorage.clear();
      llvm::StringRef KeyStr = KeyScalar->getValue(StringStorage);
      if ( !StringStorage.empty() ) {
        // Copy string to permanent storage
        unsigned Len = StringStorage.size();
        char* Buf = Allocator.Allocate<char>(Len);
        memcpy(Buf, &StringStorage[0], Len);
        KeyStr = StringRef(Buf, Len);
      }
     HNode *ValueHNode = this->createHNodes(i->getValue());
      if ( EC )
        break;
      mapHNode->Mapping[KeyStr] = ValueHNode;
    }
    return mapHNode;
  }
  else if ( llvm::isa<NullNode>(N) ) {
    return new (Allocator) EmptyHNode(N);
  }
  else {
    setError(N, "unknown node kind");
    return NULL;
  }
}


bool Input::MapHNode::isValidKey(StringRef Key) {
  for (SmallVector<const char*, 6>::iterator i=ValidKeys.begin(),
                                  End=ValidKeys.end(); i != End; ++i) {
    if ( Key.equals(*i) )
      return true;
  }
  return false;
}

void Input::setError(const Twine &Message) {
  this->setError(CurrentNode, Message);
}


//===----------------------------------------------------------------------===//
//  Output
//===----------------------------------------------------------------------===//

Output::Output(llvm::raw_ostream &yout, void *context)
    : IO(context), Out(yout), Column(0), ColumnAtFlowStart(0),
       NeedBitValueComma(false), NeedFlowSequenceComma(false),
       EnumerationMatchFound(false), NeedsNewLine(false) {
}

Output::~Output() {
}

bool Output::outputting() {
  return true;
}

void Output::beginMapping() {
  StateStack.push_back(inMapFirstKey);
  NeedsNewLine = true;
}

void Output::endMapping() {
  StateStack.pop_back();
}


bool Output::preflightKey(const char *Key, bool Required, bool SameAsDefault,
                                                bool &UseDefault, void *&) {
  UseDefault = false;
  if ( Required || !SameAsDefault ) {
    this->newLineCheck();
    this->paddedKey(Key);
    return true;
  }
  return false;
}

void Output::postflightKey(void*) {
  if ( StateStack.back() == inMapFirstKey ) {
    StateStack.pop_back();
    StateStack.push_back(inMapOtherKey);
  }
}

void Output::beginDocuments() {
  this->outputUpToEndOfLine("---");
}

bool Output::preflightDocument(unsigned index) {
  if ( index > 0 )
    this->outputUpToEndOfLine("\n---");
  return true;
}

void Output::postflightDocument() {
}

void Output::endDocuments() {
  output("\n...\n");
}

unsigned Output::beginSequence() {
  StateStack.push_back(inSeq);
  NeedsNewLine = true;
  return 0;
}
void Output::endSequence() {
  StateStack.pop_back();
}
bool Output::preflightElement(unsigned , void *&) {
  return true;
}
void Output::postflightElement(void*) {
}

unsigned Output::beginFlowSequence() {
  this->newLineCheck();
  StateStack.push_back(inFlowSeq);
  ColumnAtFlowStart = Column;
  output("[ ");
  NeedFlowSequenceComma = false;
  return 0;
}
void Output::endFlowSequence() {
  StateStack.pop_back();
  this->outputUpToEndOfLine(" ]");
}
bool Output::preflightFlowElement(unsigned , void *&) {
  if ( NeedFlowSequenceComma )
    output(", ");
  if ( Column > 70 ) {
    output("\n");
    for(int  i=0; i < ColumnAtFlowStart; ++i)
      output(" ");
    Column = ColumnAtFlowStart;
    output("  ");
  }
  return true;
}
void Output::postflightFlowElement(void*) {
  NeedFlowSequenceComma = true;
}



void Output::beginEnumScalar() {
  EnumerationMatchFound = false;
}

bool Output::matchEnumScalar(const char *Str, bool Match) {
  if ( Match && !EnumerationMatchFound ) {
    this->newLineCheck();
    this->outputUpToEndOfLine(Str);
    EnumerationMatchFound = true;
  }
  return false;
}

void Output::endEnumScalar() {
  if ( !EnumerationMatchFound )
    llvm_unreachable("bad runtime enum value");
}



bool Output::beginBitSetScalar(bool &DoClear) {
  this->newLineCheck();
  output("[ ");
  NeedBitValueComma = false;
  DoClear = false;
  return true;
}

bool Output::bitSetMatch(const char *Str, bool Matches) {
 if ( Matches ) {
    if ( NeedBitValueComma )
      output(", ");
    this->output(Str);
    NeedBitValueComma = true;
  }
  return false;
}

void Output::endBitSetScalar() {
  this->outputUpToEndOfLine(" ]");
}

void Output::scalarString(StringRef &S) {
  this->newLineCheck();
  if (S.find('\n') == StringRef::npos) {
    // No embedded new-line chars, just print string.
    this->outputUpToEndOfLine(S);
    return;
  }
  unsigned i = 0;
  unsigned j = 0;
  unsigned End = S.size();
  output("'"); // Starting single quote.
  const char *Base = S.data();
  while (j < End) {
    // Escape a single quote by doubling it.
    if (S[j] == '\'') {
      output(StringRef(&Base[i], j - i + 1));
      output("'");
      i = j + 1;
    }
    ++j;
  }
  output(StringRef(&Base[i], j - i));
  this->outputUpToEndOfLine("'"); // Ending single quote.
}

void Output::setError(const Twine &message) {
}


void Output::output(StringRef s) {
  Column += s.size();
  Out << s;
}

void Output::outputUpToEndOfLine(StringRef s) {
  this->output(s);
  if ( StateStack.back() != inFlowSeq )
    NeedsNewLine = true;
}

void Output::outputNewLine() {
  Out << "\n";
  Column = 0;
}

// if seq at top, indent as if map, then add "- "
// if seq in middle, use "- " if firstKey, else use "  "
//

void Output::newLineCheck() {
  if ( ! NeedsNewLine )
    return;
  NeedsNewLine = false;

  this->outputNewLine();

  assert(StateStack.size() > 0);
  unsigned Indent = StateStack.size() - 1;
  bool OutputDash = false;

  if ( StateStack.back() == inSeq ) {
    OutputDash = true;
  }
  else if ( (StateStack.size() > 1)
            && (StateStack.back() == inMapFirstKey)
            && (StateStack[StateStack.size()-2] == inSeq) ) {
    --Indent;
    OutputDash = true;
  }

  for (unsigned i=0; i < Indent; ++i) {
    output("  ");
  }
  if ( OutputDash ) {
    output("- ");
  }

}

void Output::paddedKey(StringRef key) {
  output(key);
  output(":");
  const char *spaces = "                ";
  if ( key.size() < strlen(spaces) )
    output(&spaces[key.size()]);
  else
    output(" ");
}

//===----------------------------------------------------------------------===//
//  traits for built-in types
//===----------------------------------------------------------------------===//

template<>
struct ScalarTraits<bool> {
  static void output(const bool &Val, void*, llvm::raw_ostream &Out) {
    Out << ( Val ? "true" : "false");
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, bool &Val) {
    if ( Scalar.equals("true") ) {
      Val = true;
      return StringRef();
    }
    else if ( Scalar.equals("false") ) {
      Val = false;
      return StringRef();
    }
    return "invalid boolean";
  }
};


template<>
struct ScalarTraits<StringRef> {
  static void output(const StringRef &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, StringRef &Val){
    Val = Scalar;
    return StringRef();
  }
};


template<>
struct ScalarTraits<uint8_t> {
  static void output(const uint8_t &Val, void*, llvm::raw_ostream &Out) {
    // use temp uin32_t because ostream thinks uint8_t is a character
    uint32_t Num = Val;
    Out << Num;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, uint8_t &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( n > 0xFF )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<uint16_t> {
  static void output(const uint16_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, uint16_t &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( n > 0xFFFF )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};

template<>
struct ScalarTraits<uint32_t> {
  static void output(const uint32_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, uint32_t &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( n > 0xFFFFFFFFUL )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<uint64_t> {
  static void output(const uint64_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, uint64_t &Val) {
    if ( getAsUnsignedInteger(Scalar, 0, Val) )
      return "invalid number";
    return StringRef();
  }
};


template<>
struct ScalarTraits<int8_t> {
  static void output(const int8_t &Val, void*, llvm::raw_ostream &Out) {
    // use temp in32_t because ostream thinks int8_t is a character
    int32_t Num = Val;
    Out << Num;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, int8_t &Val) {
    int64_t n;
    if ( getAsSignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( (n > 127) || (n < -128) )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<int16_t> {
  static void output(const int16_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, int16_t &Val) {
    int64_t n;
    if ( getAsSignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( (n > INT16_MAX) || (n < INT16_MIN) )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<int32_t> {
  static void output(const int32_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, int32_t &Val) {
    int64_t n;
    if ( getAsSignedInteger(Scalar, 0, n) )
      return "invalid number";
    if ( (n > INT32_MAX) || (n < INT32_MIN) )
      return "out of range number";
    Val = n;
    return StringRef();
  }
};

template<>
struct ScalarTraits<int64_t> {
  static void output(const int64_t &Val, void*, llvm::raw_ostream &Out) {
    Out << Val;
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, int64_t &Val) {
    if ( getAsSignedInteger(Scalar, 0, Val) )
      return "invalid number";
    return StringRef();
  }
};

template<>
struct ScalarTraits<double> {
  static void output(const double &Val, void*, llvm::raw_ostream &Out) {
  Out << format("%g", Val);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, double &Val) {
    SmallString<32> buff(Scalar.begin(), Scalar.end());
    char *end;
    Val = strtod(buff.c_str(), &end);
    if ( *end != '\0' )
      return "invalid floating point number";
    return StringRef();
  }
};

template<>
struct ScalarTraits<float> {
  static void output(const float &Val, void*, llvm::raw_ostream &Out) {
  Out << format("%g", Val);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, float &Val) {
    SmallString<32> buff(Scalar.begin(), Scalar.end());
    char *end;
    Val = strtod(buff.c_str(), &end);
    if ( *end != '\0' )
      return "invalid floating point number";
    return StringRef();
  }
};



template<>
struct ScalarTraits<Hex8> {
  static void output(const Hex8 &Val, void*, llvm::raw_ostream &Out) {
    uint8_t Num = Val;
    Out << format("0x%02X", Num);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, Hex8 &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid hex8 number";
    if ( n > 0xFF )
      return "out of range hex8 number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<Hex16> {
  static void output(const Hex16 &Val, void*, llvm::raw_ostream &Out) {
    uint16_t Num = Val;
    Out << format("0x%04X", Num);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, Hex16 &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid hex16 number";
    if ( n > 0xFFFF )
      return "out of range hex16 number";
    Val = n;
    return StringRef();
  }
};

template<>
struct ScalarTraits<Hex32> {
  static void output(const Hex32 &Val, void*, llvm::raw_ostream &Out) {
    uint32_t Num = Val;
    Out << format("0x%08X", Num);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, Hex32 &Val) {
    uint64_t n;
    if ( getAsUnsignedInteger(Scalar, 0, n) )
      return "invalid hex32 number";
    if ( n > 0xFFFFFFFFUL )
      return "out of range hex32 number";
    Val = n;
    return StringRef();
  }
};


template<>
struct ScalarTraits<Hex64> {
  static void output(const Hex64 &Val, void*, llvm::raw_ostream &Out) {
    uint64_t Num = Val;
    Out << format("0x%016llX", Num);
  }
  static llvm::StringRef input(llvm::StringRef Scalar, void*, Hex64 &Val) {
    uint64_t Num;
    if ( getAsUnsignedInteger(Scalar, 0, Num) )
      return "invalid hex64 number";
    Val = Num;
    return StringRef();
  }
};




// We want all the ScalarTrait specialized on built-in types
// to be instantiated here.
template <typename T>
struct ForceUse {
  ForceUse() : oproc(ScalarTraits<T>::output), iproc(ScalarTraits<T>::input) {}
  void (*oproc)(const T &, void*, llvm::raw_ostream &);
  llvm::StringRef (*iproc)(llvm::StringRef, void*, T &);
};

static ForceUse<bool>            Dummy1;
static ForceUse<llvm::StringRef> Dummy2;
static ForceUse<uint8_t>         Dummy3;
static ForceUse<uint16_t>        Dummy4;
static ForceUse<uint32_t>        Dummy5;
static ForceUse<uint64_t>        Dummy6;
static ForceUse<int8_t>          Dummy7;
static ForceUse<int16_t>         Dummy8;
static ForceUse<int32_t>         Dummy9;
static ForceUse<int64_t>         Dummy10;
static ForceUse<float>           Dummy11;
static ForceUse<double>          Dummy12;
static ForceUse<Hex8>            Dummy13;
static ForceUse<Hex16>           Dummy14;
static ForceUse<Hex32>           Dummy15;
static ForceUse<Hex64>           Dummy16;



} // namespace yaml
} // namespace llvm


