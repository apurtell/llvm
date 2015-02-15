//===- ValueMap.h - Safe map from Values to data ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ValueMap class.  ValueMap maps Value* or any subclass
// to an arbitrary other type.  It provides the DenseMap interface but updates
// itself to remain safe when keys are RAUWed or deleted.  By default, when a
// key is RAUWed from V1 to V2, the old mapping V1->target is removed, and a new
// mapping V2->target is added.  If V2 already existed, its old target is
// overwritten.  When a key is deleted, its mapping is removed.
//
// You can override a ValueMap's Config parameter to control exactly what
// happens on RAUW and destruction and to get called back on each event.  It's
// legal to call back into the ValueMap from a Config's callbacks.  Config
// parameters should inherit from ValueMapConfig<KeyT> to get default
// implementations of all the methods ValueMap uses.  See ValueMapConfig for
// documentation of the functions you can override.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_VALUEMAP_H
#define LLVM_IR_VALUEMAP_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/TrackingMDRef.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/Mutex.h"
#include "llvm/Support/UniqueLock.h"
#include "llvm/Support/type_traits.h"
#include <iterator>
#include <memory>

namespace llvm {

template<typename KeyT, typename ValueT, typename Config>
class ValueMapCallbackVH;

template<typename DenseMapT, typename KeyT>
class ValueMapIterator;
template<typename DenseMapT, typename KeyT>
class ValueMapConstIterator;

/// This class defines the default behavior for configurable aspects of
/// ValueMap<>.  User Configs should inherit from this class to be as compatible
/// as possible with future versions of ValueMap.
template<typename KeyT, typename MutexT = sys::Mutex>
struct ValueMapConfig {
  typedef MutexT mutex_type;

  /// If FollowRAUW is true, the ValueMap will update mappings on RAUW. If it's
  /// false, the ValueMap will leave the original mapping in place.
  enum { FollowRAUW = true };

  // All methods will be called with a first argument of type ExtraData.  The
  // default implementations in this class take a templated first argument so
  // that users' subclasses can use any type they want without having to
  // override all the defaults.
  struct ExtraData {};

  template<typename ExtraDataT>
  static void onRAUW(const ExtraDataT & /*Data*/, KeyT /*Old*/, KeyT /*New*/) {}
  template<typename ExtraDataT>
  static void onDelete(const ExtraDataT &/*Data*/, KeyT /*Old*/) {}

  /// Returns a mutex that should be acquired around any changes to the map.
  /// This is only acquired from the CallbackVH (and held around calls to onRAUW
  /// and onDelete) and not inside other ValueMap methods.  NULL means that no
  /// mutex is necessary.
  template<typename ExtraDataT>
  static mutex_type *getMutex(const ExtraDataT &/*Data*/) { return nullptr; }
};

/// See the file comment.
template<typename KeyT, typename ValueT, typename Config =ValueMapConfig<KeyT> >
class ValueMap {
  friend class ValueMapCallbackVH<KeyT, ValueT, Config>;
  typedef ValueMapCallbackVH<KeyT, ValueT, Config> ValueMapCVH;
  typedef DenseMap<ValueMapCVH, ValueT, DenseMapInfo<ValueMapCVH> > MapT;
  typedef DenseMap<const Metadata *, TrackingMDRef> MDMapT;
  typedef typename Config::ExtraData ExtraData;
  MapT Map;
  std::unique_ptr<MDMapT> MDMap;
  ExtraData Data;
  ValueMap(const ValueMap&) = delete;
  ValueMap& operator=(const ValueMap&) = delete;
public:
  typedef KeyT key_type;
  typedef ValueT mapped_type;
  typedef std::pair<KeyT, ValueT> value_type;
  typedef unsigned size_type;

  explicit ValueMap(unsigned NumInitBuckets = 64)
      : Map(NumInitBuckets), Data() {}
  explicit ValueMap(const ExtraData &Data, unsigned NumInitBuckets = 64)
      : Map(NumInitBuckets), Data(Data) {}

  ~ValueMap() {}

  bool hasMD() const { return MDMap; }
  MDMapT &MD() {
    if (!MDMap)
      MDMap.reset(new MDMapT);
    return *MDMap;
  }

  typedef ValueMapIterator<MapT, KeyT> iterator;
  typedef ValueMapConstIterator<MapT, KeyT> const_iterator;
  inline iterator begin() { return iterator(Map.begin()); }
  inline iterator end() { return iterator(Map.end()); }
  inline const_iterator begin() const { return const_iterator(Map.begin()); }
  inline const_iterator end() const { return const_iterator(Map.end()); }

  bool empty() const { return Map.empty(); }
  size_type size() const { return Map.size(); }

  /// Grow the map so that it has at least Size buckets. Does not shrink
  void resize(size_t Size) { Map.resize(Size); }

  void clear() {
    Map.clear();
    MDMap.reset();
  }

  /// Return 1 if the specified key is in the map, 0 otherwise.
  size_type count(const KeyT &Val) const {
    return Map.find_as(Val) == Map.end() ? 0 : 1;
  }

  iterator find(const KeyT &Val) {
    return iterator(Map.find_as(Val));
  }
  const_iterator find(const KeyT &Val) const {
    return const_iterator(Map.find_as(Val));
  }

  /// lookup - Return the entry for the specified key, or a default
  /// constructed value if no such entry exists.
  ValueT lookup(const KeyT &Val) const {
    typename MapT::const_iterator I = Map.find_as(Val);
    return I != Map.end() ? I->second : ValueT();
  }

  // Inserts key,value pair into the map if the key isn't already in the map.
  // If the key is already in the map, it returns false and doesn't update the
  // value.
  std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT> &KV) {
    std::pair<typename MapT::iterator, bool> map_result=
      Map.insert(std::make_pair(Wrap(KV.first), KV.second));
    return std::make_pair(iterator(map_result.first), map_result.second);
  }

  /// insert - Range insertion of pairs.
  template<typename InputIt>
  void insert(InputIt I, InputIt E) {
    for (; I != E; ++I)
      insert(*I);
  }


  bool erase(const KeyT &Val) {
    typename MapT::iterator I = Map.find_as(Val);
    if (I == Map.end())
      return false;

    Map.erase(I);
    return true;
  }
  void erase(iterator I) {
    return Map.erase(I.base());
  }

  value_type& FindAndConstruct(const KeyT &Key) {
    return Map.FindAndConstruct(Wrap(Key));
  }

  ValueT &operator[](const KeyT &Key) {
    return Map[Wrap(Key)];
  }

  /// isPointerIntoBucketsArray - Return true if the specified pointer points
  /// somewhere into the ValueMap's array of buckets (i.e. either to a key or
  /// value in the ValueMap).
  bool isPointerIntoBucketsArray(const void *Ptr) const {
    return Map.isPointerIntoBucketsArray(Ptr);
  }

  /// getPointerIntoBucketsArray() - Return an opaque pointer into the buckets
  /// array.  In conjunction with the previous method, this can be used to
  /// determine whether an insertion caused the ValueMap to reallocate.
  const void *getPointerIntoBucketsArray() const {
    return Map.getPointerIntoBucketsArray();
  }

private:
  // Takes a key being looked up in the map and wraps it into a
  // ValueMapCallbackVH, the actual key type of the map.  We use a helper
  // function because ValueMapCVH is constructed with a second parameter.
  ValueMapCVH Wrap(KeyT key) const {
    // The only way the resulting CallbackVH could try to modify *this (making
    // the const_cast incorrect) is if it gets inserted into the map.  But then
    // this function must have been called from a non-const method, making the
    // const_cast ok.
    return ValueMapCVH(key, const_cast<ValueMap*>(this));
  }
};

// This CallbackVH updates its ValueMap when the contained Value changes,
// according to the user's preferences expressed through the Config object.
template<typename KeyT, typename ValueT, typename Config>
class ValueMapCallbackVH : public CallbackVH {
  friend class ValueMap<KeyT, ValueT, Config>;
  friend struct DenseMapInfo<ValueMapCallbackVH>;
  typedef ValueMap<KeyT, ValueT, Config> ValueMapT;
  typedef typename std::remove_pointer<KeyT>::type KeySansPointerT;

  ValueMapT *Map;

  ValueMapCallbackVH(KeyT Key, ValueMapT *Map)
      : CallbackVH(const_cast<Value*>(static_cast<const Value*>(Key))),
        Map(Map) {}

  // Private constructor used to create empty/tombstone DenseMap keys.
  ValueMapCallbackVH(Value *V) : CallbackVH(V), Map(nullptr) {}

public:
  KeyT Unwrap() const { return cast_or_null<KeySansPointerT>(getValPtr()); }

  void deleted() override {
    // Make a copy that won't get changed even when *this is destroyed.
    ValueMapCallbackVH Copy(*this);
    typename Config::mutex_type *M = Config::getMutex(Copy.Map->Data);
    unique_lock<typename Config::mutex_type> Guard;
    if (M)
      Guard = unique_lock<typename Config::mutex_type>(*M);
    Config::onDelete(Copy.Map->Data, Copy.Unwrap());  // May destroy *this.
    Copy.Map->Map.erase(Copy);  // Definitely destroys *this.
  }
  void allUsesReplacedWith(Value *new_key) override {
    assert(isa<KeySansPointerT>(new_key) &&
           "Invalid RAUW on key of ValueMap<>");
    // Make a copy that won't get changed even when *this is destroyed.
    ValueMapCallbackVH Copy(*this);
    typename Config::mutex_type *M = Config::getMutex(Copy.Map->Data);
    unique_lock<typename Config::mutex_type> Guard;
    if (M)
      Guard = unique_lock<typename Config::mutex_type>(*M);

    KeyT typed_new_key = cast<KeySansPointerT>(new_key);
    // Can destroy *this:
    Config::onRAUW(Copy.Map->Data, Copy.Unwrap(), typed_new_key);
    if (Config::FollowRAUW) {
      typename ValueMapT::MapT::iterator I = Copy.Map->Map.find(Copy);
      // I could == Copy.Map->Map.end() if the onRAUW callback already
      // removed the old mapping.
      if (I != Copy.Map->Map.end()) {
        ValueT Target(I->second);
        Copy.Map->Map.erase(I);  // Definitely destroys *this.
        Copy.Map->insert(std::make_pair(typed_new_key, Target));
      }
    }
  }
};

template<typename KeyT, typename ValueT, typename Config>
struct DenseMapInfo<ValueMapCallbackVH<KeyT, ValueT, Config> > {
  typedef ValueMapCallbackVH<KeyT, ValueT, Config> VH;

  static inline VH getEmptyKey() {
    return VH(DenseMapInfo<Value *>::getEmptyKey());
  }
  static inline VH getTombstoneKey() {
    return VH(DenseMapInfo<Value *>::getTombstoneKey());
  }
  static unsigned getHashValue(const VH &Val) {
    return DenseMapInfo<KeyT>::getHashValue(Val.Unwrap());
  }
  static unsigned getHashValue(const KeyT &Val) {
    return DenseMapInfo<KeyT>::getHashValue(Val);
  }
  static bool isEqual(const VH &LHS, const VH &RHS) {
    return LHS == RHS;
  }
  static bool isEqual(const KeyT &LHS, const VH &RHS) {
    return LHS == RHS.getValPtr();
  }
};


template<typename DenseMapT, typename KeyT>
class ValueMapIterator :
    public std::iterator<std::forward_iterator_tag,
                         std::pair<KeyT, typename DenseMapT::mapped_type>,
                         ptrdiff_t> {
  typedef typename DenseMapT::iterator BaseT;
  typedef typename DenseMapT::mapped_type ValueT;
  BaseT I;
public:
  ValueMapIterator() : I() {}

  ValueMapIterator(BaseT I) : I(I) {}

  BaseT base() const { return I; }

  struct ValueTypeProxy {
    const KeyT first;
    ValueT& second;
    ValueTypeProxy *operator->() { return this; }
    operator std::pair<KeyT, ValueT>() const {
      return std::make_pair(first, second);
    }
  };

  ValueTypeProxy operator*() const {
    ValueTypeProxy Result = {I->first.Unwrap(), I->second};
    return Result;
  }

  ValueTypeProxy operator->() const {
    return operator*();
  }

  bool operator==(const ValueMapIterator &RHS) const {
    return I == RHS.I;
  }
  bool operator!=(const ValueMapIterator &RHS) const {
    return I != RHS.I;
  }

  inline ValueMapIterator& operator++() {  // Preincrement
    ++I;
    return *this;
  }
  ValueMapIterator operator++(int) {  // Postincrement
    ValueMapIterator tmp = *this; ++*this; return tmp;
  }
};

template<typename DenseMapT, typename KeyT>
class ValueMapConstIterator :
    public std::iterator<std::forward_iterator_tag,
                         std::pair<KeyT, typename DenseMapT::mapped_type>,
                         ptrdiff_t> {
  typedef typename DenseMapT::const_iterator BaseT;
  typedef typename DenseMapT::mapped_type ValueT;
  BaseT I;
public:
  ValueMapConstIterator() : I() {}
  ValueMapConstIterator(BaseT I) : I(I) {}
  ValueMapConstIterator(ValueMapIterator<DenseMapT, KeyT> Other)
    : I(Other.base()) {}

  BaseT base() const { return I; }

  struct ValueTypeProxy {
    const KeyT first;
    const ValueT& second;
    ValueTypeProxy *operator->() { return this; }
    operator std::pair<KeyT, ValueT>() const {
      return std::make_pair(first, second);
    }
  };

  ValueTypeProxy operator*() const {
    ValueTypeProxy Result = {I->first.Unwrap(), I->second};
    return Result;
  }

  ValueTypeProxy operator->() const {
    return operator*();
  }

  bool operator==(const ValueMapConstIterator &RHS) const {
    return I == RHS.I;
  }
  bool operator!=(const ValueMapConstIterator &RHS) const {
    return I != RHS.I;
  }

  inline ValueMapConstIterator& operator++() {  // Preincrement
    ++I;
    return *this;
  }
  ValueMapConstIterator operator++(int) {  // Postincrement
    ValueMapConstIterator tmp = *this; ++*this; return tmp;
  }
};

} // end namespace llvm

#endif
