/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Originally these classes are from Chromium.
// http://src.chromium.org/viewvc/chrome/trunk/src/base/memory/ref_counted.h?view=markup

//
// A smart pointer class for reference counted objects.  Use this class instead
// of calling AddRef and Release manually on a reference counted object to
// avoid common memory leaks caused by forgetting to Release an object
// reference.  Sample usage:
//
//   class MyFoo : public RefCounted<MyFoo> {
//    ...
//   };
//
//   void some_function() {
//     scoped_refptr<MyFoo> foo = new MyFoo();
//     foo->Method(param);
//     // |foo| is released when this function returns
//   }
//
//   void some_other_function() {
//     scoped_refptr<MyFoo> foo = new MyFoo();
//     ...
//     foo = nullptr;  // explicitly releases |foo|
//     ...
//     if (foo)
//       foo->Method(param);
//   }
//
// The above examples show how scoped_refptr<T> acts like a pointer to T.
// Given two scoped_refptr<T> classes, it is also possible to exchange
// references between the two objects, like so:
//
//   {
//     scoped_refptr<MyFoo> a = new MyFoo();
//     scoped_refptr<MyFoo> b;
//
//     b.swap(a);
//     // now, |b| references the MyFoo object, and |a| references null.
//   }
//
// To make both |a| and |b| in the above example reference the same MyFoo
// object, simply use the assignment operator:
//
//   {
//     scoped_refptr<MyFoo> a = new MyFoo();
//     scoped_refptr<MyFoo> b;
//
//     b = a;
//     // now, |a| and |b| each own a reference to the same MyFoo object.
//   }
//

#ifndef API_SCOPED_REFPTR_H_
#define API_SCOPED_REFPTR_H_

#include <memory>
#include <utility>

namespace rtc {

template <class T>
class scoped_refptr {
 public:
  typedef T element_type;
  //默认构造 传入空的引用计数对象
  scoped_refptr() : ptr_(nullptr) {}
  //构造 传入引用计数对象p的指针，调用其AddRef()方法 引用计数+1
  scoped_refptr(T* p) : ptr_(p) {  // NOLINT(runtime/explicit)
    if (ptr_)
      ptr_->AddRef();
  }
  //拷贝构造，对应的引用计数+1，调用引用计数对象的AddRef()方法
  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_)
      ptr_->AddRef();
  }
  //拷贝构造，U必须是T的子类
  template <typename U>
  scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
    if (ptr_)
      ptr_->AddRef();
  }
  //移动构造(右值引用)，表示对象的转移，亦即使用同一个对象.因此，需要保持对引用计数对象的引用次数不变
  //所以此处调用scoped_refptr<T>的release()方法，将原来的scoped_refptr<T>对象内部引用计数指针置空
  //新的scoped_refptr<T>对象来保存引用计数对象，以达到转移的目的
  // Move constructors.
  scoped_refptr(scoped_refptr<T>&& r) noexcept : ptr_(r.release()) {}
  //移动构造，U必须是T的子类
  template <typename U>
  scoped_refptr(scoped_refptr<U>&& r) noexcept : ptr_(r.release()) {}
  //析构函数，调用引用计数对象Release()，引用计数-1
  ~scoped_refptr() {
    if (ptr_)
      ptr_->Release();
  }

  T* get() const { return ptr_; }
  operator T*() const { return ptr_; }
  T* operator->() const { return ptr_; }
  //不可轻易调用.会使智能指针内部存储的ptr_为空,失去对引用计数对象的引用,而引用计数没有-1
  // Returns the (possibly null) raw pointer, and makes the scoped_refptr hold a
  // null pointer, all without touching the reference count of the underlying
  // pointed-to object. The object is still reference counted, and the caller of
  // release() is now the proud owner of one reference, so it is responsible for
  // calling Release() once on the object when no longer using it.
  T* release() {
    T* retVal = ptr_;
    ptr_ = nullptr;
    return retVal;
  }
  //重载赋值运算符.赋值新的引用计数对象的指针，新引用计数对象的引用计数+1，原来的-1
  scoped_refptr<T>& operator=(T* p) {
    // AddRef first so that self assignment should work
    if (p)
      p->AddRef();
    if (ptr_)
      ptr_->Release();
    ptr_ = p;
    return *this;
  }
  //赋值智能指针，新引用计数对象的引用计数+1，原来的-1
  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_; //取出智能指针的内部引用计数的指针,利用重载赋值运算符的功能实现赋值
  }
  //赋值T的子类U的智能指针，新引用计数对象的引用计数+1，原来的-1
  template <typename U>
  scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
    return *this = r.get();//使用get()方法取出智能指针的内部引用计数的指针，利用重载赋值运算符的功能实现赋值
  }
  //移动赋值右值智能指针，新引用计数对象的引用计数不变，原来引用计数对象的引用计数不变
  //使用移动语义std::move + 移动构造 + swap进行引用计数对象的地址交换
  scoped_refptr<T>& operator=(scoped_refptr<T>&& r) noexcept {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }
  //移动赋值T的子类U的右值智能指针，新引用计数对象的引用计数不变，原来引用计数对象的引用计数不变
  //使用移动语义std::move + 移动构造 + swap进行引用计数对象的地址交换
  template <typename U>
  scoped_refptr<T>& operator=(scoped_refptr<U>&& r) noexcept {
    scoped_refptr<T>(std::move(r)).swap(*this);
    return *this;
  }
  //地址交换函数. 引用计数对象的地址交换
  void swap(T** pp) noexcept {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }
  //智能智能内部的引用计数对象的地址交换
  void swap(scoped_refptr<T>& r) noexcept { swap(&r.ptr_); }

 protected:
  T* ptr_;
};

}  // namespace rtc

#endif  // API_SCOPED_REFPTR_H_
