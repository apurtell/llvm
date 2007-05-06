//===-- llvm/Target/TargetData.h - Data size & alignment info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines target properties related to datatype size/offset/alignment
// information.  It uses lazy annotations to cache information about how
// structure types are laid out and used.
//
// This structure should be created once, filled in if the defaults are not
// correct and then passed around by const&.  None of the members functions
// require modification to the object.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_TARGETDATA_H
#define LLVM_TARGET_TARGETDATA_H

#include "llvm/Pass.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/ADT/SmallVector.h"
#include <string>

namespace llvm {

class Value;
class Type;
class StructType;
class StructLayout;
class GlobalVariable;

/// Enum used to categorize the alignment types stored by TargetAlignElem
enum AlignTypeEnum {
  INTEGER_ALIGN = 'i',               ///< Integer type alignment
  VECTOR_ALIGN = 'v',                ///< Vector type alignment
  FLOAT_ALIGN = 'f',                 ///< Floating point type alignment
  AGGREGATE_ALIGN = 'a'              ///< Aggregate alignment
};
/// Target alignment element.
///
/// Stores the alignment data associated with a given alignment type (pointer,
/// integer, packed/vector, float) and type bit width.
///
/// @note The unusual order of elements in the structure attempts to reduce
/// padding and make the structure slightly more cache friendly.
struct TargetAlignElem {
  AlignTypeEnum       AlignType : 8;  //< Alignment type (AlignTypeEnum)
  unsigned char       ABIAlign;       //< ABI alignment for this type/bitw
  unsigned char       PrefAlign;      //< Pref. alignment for this type/bitw
  uint32_t            TypeBitWidth;   //< Type bit width

  /// Initializer
  static TargetAlignElem get(AlignTypeEnum align_type, unsigned char abi_align,
                             unsigned char pref_align, uint32_t bit_width);
  /// Equality predicate
  bool operator==(const TargetAlignElem &rhs) const;
  /// output stream operator
  std::ostream &dump(std::ostream &os) const;
};

class TargetData : public ImmutablePass {
private:
  bool          LittleEndian;          ///< Defaults to false
  unsigned char PointerMemSize;        ///< Pointer size in bytes
  unsigned char PointerABIAlign;       ///< Pointer ABI alignment
  unsigned char PointerPrefAlign;      ///< Pointer preferred alignment

  //! Where the primitive type alignment data is stored.
  /*!
   @sa init().
   @note Could support multiple size pointer alignments, e.g., 32-bit pointers
   vs. 64-bit pointers by extending TargetAlignment, but for now, we don't.
   */
  SmallVector<TargetAlignElem, 16> Alignments;
  //! Alignment iterator shorthand
  typedef SmallVector<TargetAlignElem, 16>::iterator align_iterator;
  //! Constant alignment iterator shorthand
  typedef SmallVector<TargetAlignElem, 16>::const_iterator align_const_iterator;
  //! Invalid alignment.
  /*!
    This member is a signal that a requested alignment type and bit width were
    not found in the SmallVector.
   */
  static const TargetAlignElem InvalidAlignmentElem;

  //! Set/initialize target alignments
  void setAlignment(AlignTypeEnum align_type, unsigned char abi_align,
                    unsigned char pref_align, uint32_t bit_width);
  unsigned getAlignmentInfo(AlignTypeEnum align_type, uint32_t bit_width,
                            bool ABIAlign) const;
  //! Internal helper method that returns requested alignment for type.
  unsigned char getAlignment(const Type *Ty, bool abi_or_pref) const;

  /// Valid alignment predicate.
  ///
  /// Predicate that tests a TargetAlignElem reference returned by get() against
  /// InvalidAlignmentElem.
  inline bool validAlignment(const TargetAlignElem &align) const {
    return (&align != &InvalidAlignmentElem);
  }

public:
  /// Default ctor.
  ///
  /// @note This has to exist, because this is a pass, but it should never be
  /// used.
  TargetData() : ImmutablePass((intptr_t)&ID) {
    assert(0 && "ERROR: Bad TargetData ctor used.  "
           "Tool did not specify a TargetData to use?");
    abort();
  }
    
  /// Constructs a TargetData from a specification string. See init().
  TargetData(const std::string &TargetDescription) 
    : ImmutablePass((intptr_t)&ID) {
    init(TargetDescription);
  }

  /// Initialize target data from properties stored in the module.
  TargetData(const Module *M);

  TargetData(const TargetData &TD) : 
    ImmutablePass((intptr_t)&ID),
    LittleEndian(TD.isLittleEndian()),
    PointerMemSize(TD.PointerMemSize),
    PointerABIAlign(TD.PointerABIAlign),
    PointerPrefAlign(TD.PointerPrefAlign),
    Alignments(TD.Alignments)
  { }

  ~TargetData();  // Not virtual, do not subclass this class

  //! Parse a target data layout string and initialize TargetData alignments.
  void init(const std::string &TargetDescription);
  
  /// Target endianness...
  bool          isLittleEndian()       const { return     LittleEndian; }
  bool          isBigEndian()          const { return    !LittleEndian; }

  /// getStringRepresentation - Return the string representation of the
  /// TargetData.  This representation is in the same format accepted by the
  /// string constructor above.
  std::string getStringRepresentation() const;
  /// Target pointer alignment
  unsigned char getPointerABIAlignment() const { return PointerABIAlign; }
  /// Return target's alignment for stack-based pointers
  unsigned char getPointerPrefAlignment() const { return PointerPrefAlign; }
  /// Target pointer size
  unsigned char getPointerSize()         const { return PointerMemSize; }
  /// Target pointer size, in bits
  unsigned char getPointerSizeInBits()   const { return 8*PointerMemSize; }

  /// getTypeSize - Return the number of bytes necessary to hold the specified
  /// type.
  uint64_t getTypeSize(const Type *Ty) const;

  /// getTypeSizeInBits - Return the number of bits necessary to hold the
  /// specified type.
  uint64_t getTypeSizeInBits(const Type* Ty) const;

  /// getABITypeAlignment - Return the minimum ABI-required alignment for the
  /// specified type.
  unsigned char getABITypeAlignment(const Type *Ty) const;

  /// getPrefTypeAlignment - Return the preferred stack/global alignment for
  /// the specified type.
  unsigned char getPrefTypeAlignment(const Type *Ty) const;

  /// getPreferredTypeAlignmentShift - Return the preferred alignment for the
  /// specified type, returned as log2 of the value (a shift amount).
  ///
  unsigned char getPreferredTypeAlignmentShift(const Type *Ty) const;

  /// getIntPtrType - Return an unsigned integer type that is the same size or
  /// greater to the host pointer size.
  ///
  const Type *getIntPtrType() const;

  /// getIndexedOffset - return the offset from the beginning of the type for the
  /// specified indices.  This is used to implement getelementptr.
  ///
  uint64_t getIndexedOffset(const Type *Ty,
                            Value* const* Indices, unsigned NumIndices) const;
  
  /// getStructLayout - Return a StructLayout object, indicating the alignment
  /// of the struct, its size, and the offsets of its fields.  Note that this
  /// information is lazily cached.
  const StructLayout *getStructLayout(const StructType *Ty) const;
  
  /// InvalidateStructLayoutInfo - TargetData speculatively caches StructLayout
  /// objects.  If a TargetData object is alive when types are being refined and
  /// removed, this method must be called whenever a StructType is removed to
  /// avoid a dangling pointer in this cache.
  void InvalidateStructLayoutInfo(const StructType *Ty) const;

  /// getPreferredAlignmentLog - Return the preferred alignment of the
  /// specified global, returned in log form.  This includes an explicitly
  /// requested alignment (if the global has one).
  unsigned getPreferredAlignmentLog(const GlobalVariable *GV) const;

  static char ID; // Pass identification, replacement for typeid
};

/// StructLayout - used to lazily calculate structure layout information for a
/// target machine, based on the TargetData structure.
///
class StructLayout {
  uint64_t StructSize;
  unsigned StructAlignment;
  unsigned NumElements;
  uint64_t MemberOffsets[1];  // variable sized array!
public:

  uint64_t getSizeInBytes() const {
    return StructSize;
  }
  
  unsigned getAlignment() const {
    return StructAlignment;
  }
    
  /// getElementContainingOffset - Given a valid offset into the structure,
  /// return the structure index that contains it.
  ///
  unsigned getElementContainingOffset(uint64_t Offset) const;

  uint64_t getElementOffset(unsigned Idx) const {
    assert(Idx < NumElements && "Invalid element idx!");
    return MemberOffsets[Idx];
  }
  
private:
  friend class TargetData;   // Only TargetData can create this class
  StructLayout(const StructType *ST, const TargetData &TD);
};

} // End llvm namespace

#endif
