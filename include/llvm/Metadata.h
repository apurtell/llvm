//===-- llvm/Metadata.h - Metadata definitions ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// @file
/// This file contains the declarations for metadata subclasses.
/// They represent the different flavors of metadata that live in LLVM.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MDNODE_H
#define LLVM_MDNODE_H

#include "llvm/Constant.h"
#include "llvm/Type.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ValueHandle.h"

namespace llvm {

//===----------------------------------------------------------------------===//
// MetadataBase  - A base class for MDNode and MDString.
class MetadataBase : public Value {
protected:
  MetadataBase(const Type *Ty, unsigned scid)
    : Value(Ty, scid) {}

public:
  /// getType() specialization - Type is always MetadataTy.
  ///
  inline const Type *getType() const {
    return Type::MetadataTy;
  }

  /// isNullValue - Return true if this is the value that would be returned by
  /// getNullValue.  This always returns false because getNullValue will never
  /// produce metadata.
  virtual bool isNullValue() const {
    return false;
  }

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const MDString *) { return true; }
  static bool classof(const Value *V) {
    return V->getValueID() == MDStringVal || V->getValueID() == MDNodeVal;
  }
};

//===----------------------------------------------------------------------===//
/// MDString - a single uniqued string.
/// These are used to efficiently contain a byte sequence for metadata.
///
class MDString : public MetadataBase {
  MDString(const MDString &);            // DO NOT IMPLEMENT
  StringRef Str;
  friend class LLVMContextImpl;

protected:
  explicit MDString(const char *begin, unsigned l)
    : MetadataBase(Type::MetadataTy, Value::MDStringVal), Str(begin, l) {}

public:
  StringRef getString() const { return Str; }

  unsigned length() const { return Str.size(); }

  /// begin() - Pointer to the first byte of the string.
  ///
  const char *begin() const { return Str.begin(); }

  /// end() - Pointer to one byte past the end of the string.
  ///
  const char *end() const { return Str.end(); }

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const MDString *) { return true; }
  static bool classof(const Value *V) {
    return V->getValueID() == MDStringVal;
  }
};

//===----------------------------------------------------------------------===//
/// MDNode - a tuple of other values.
/// These contain a list of the values that represent the metadata. 
///
class MDNode : public MetadataBase, public FoldingSetNode {
  MDNode(const MDNode &);      // DO NOT IMPLEMENT

  friend class LLVMContextImpl;

  SmallVector<WeakVH, 4> Node;
  typedef SmallVectorImpl<WeakVH>::iterator elem_iterator;

protected:
  explicit MDNode(Value*const* Vals, unsigned NumVals);
public:
  typedef SmallVectorImpl<WeakVH>::const_iterator const_elem_iterator;

  Value *getElement(unsigned i) const {
    return Node[i];
  }

  unsigned getNumElements() const {
    return Node.size();
  }

  bool elem_empty() const {
    return Node.empty();
  }

  const_elem_iterator elem_begin() const {
    return Node.begin();
  }

  const_elem_iterator elem_end() const {
    return Node.end();
  }

  /// getType() specialization - Type is always MetadataTy.
  ///
  inline const Type *getType() const {
    return Type::MetadataTy;
  }

  /// isNullValue - Return true if this is the value that would be returned by
  /// getNullValue.  This always returns false because getNullValue will never
  /// produce metadata.
  virtual bool isNullValue() const {
    return false;
  }

  /// Profile - calculate a unique identifier for this MDNode to collapse
  /// duplicates
  void Profile(FoldingSetNodeID &ID) const;

  virtual void replaceUsesOfWithOnConstant(Value *From, Value *To, Use *U) {
    llvm_unreachable("This should never be called because MDNodes have no ops");
  }

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static inline bool classof(const MDNode *) { return true; }
  static bool classof(const Value *V) {
    return V->getValueID() == MDNodeVal;
  }
};

} // end llvm namespace

#endif
