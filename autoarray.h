/*
 * This file is part of rsyncrypto - rsync friendly encryption
 * Copyright (C) 2005 Shachar Shemesh for Lingnu Open Source Consulting (http://www.lignu.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Copyright (c) 1997-1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef _AUTOMAP_H
#define _AUTOMAP_H

#if defined(__SGI_STL_USE_auto_array_CONVERSIONS) && \
    defined(__STL_MEMBER_TEMPLATES)

template<class _Tp1> struct auto_array_ref {
  _Tp1* _M_ptr;
  auto_array_ref(_Tp1* __p) : _M_ptr(__p) {}
};

#endif

#define __STL_NOTHROW throw()

template <class _Tp> class auto_array {
private:
  _Tp* _M_ptr;

public:
  typedef _Tp element_type;

  explicit auto_array(_Tp* __p = 0) __STL_NOTHROW : _M_ptr(__p) {}
  auto_array(auto_array& __a) __STL_NOTHROW : _M_ptr(__a.release()) {}

#ifdef __STL_MEMBER_TEMPLATES
  auto_array(auto_array<_Tp>& __a) __STL_NOTHROW
    : _M_ptr(__a.release()) {}
#endif /* __STL_MEMBER_TEMPLATES */

  auto_array& operator=(auto_array& __a) __STL_NOTHROW {
    if (&__a != this) {
      delete [] _M_ptr;
      _M_ptr = __a.release();
    }
    return *this;
  }

#ifdef __STL_MEMBER_TEMPLATES
  template <class _Tp1>
  auto_array& operator=(auto_array<_Tp1>& __a) __STL_NOTHROW {
    if (__a.get() != this->get()) {
      delete [] _M_ptr;
      _M_ptr = __a.release();
    }
    return *this;
  }
#endif /* __STL_MEMBER_TEMPLATES */

  // Note: The C++ standard says there is supposed to be an empty throw
  // specification here, but omitting it is standard conforming.  Its 
  // presence can be detected only if _Tp::~_Tp() throws, but (17.4.3.6/2)
  // this is prohibited.
  ~auto_array() { delete [] _M_ptr; }

  _Tp& operator*() const __STL_NOTHROW {
    return *_M_ptr;
  }
  _Tp& operator[](size_t index) __STL_NOTHROW {
      return _M_ptr[index];
  }
  _Tp* get() const __STL_NOTHROW {
    return _M_ptr;
  }
  _Tp* release() __STL_NOTHROW {
    _Tp* __tmp = _M_ptr;
    _M_ptr = 0;
    return __tmp;
  }
  void reset(_Tp* __p = 0) __STL_NOTHROW {
    if (__p != _M_ptr) {
      delete [] _M_ptr;
      _M_ptr = __p;
    }
  }

  // According to the C++ standard, these conversions are required.  Most
  // present-day compilers, however, do not enforce that requirement---and, 
  // in fact, most present-day compilers do not support the language 
  // features that these conversions rely on.
  
#if defined(__SGI_STL_USE_auto_array_CONVERSIONS) && \
    defined(__STL_MEMBER_TEMPLATES)

public:
  auto_array(auto_array_ref<_Tp> __ref) __STL_NOTHROW
    : _M_ptr(__ref._M_ptr) {}

  auto_array& operator=(auto_array_ref<_Tp> __ref) __STL_NOTHROW {
    if (__ref._M_ptr != this->get()) {
      delete [] _M_ptr;
      _M_ptr = __ref._M_ptr;
    }
    return *this;
  }

  template <class _Tp1> operator auto_array_ref<_Tp1>() __STL_NOTHROW 
    { return auto_array_ref<_Tp1>(this->release()); }
  template <class _Tp1> operator auto_array<_Tp1>() __STL_NOTHROW
    { return auto_array<_Tp1>(this->release()); }

#endif /* auto ptr conversions && member templates */
};

#endif /* _AUTOARRAY_H */


// Local Variables:
// mode:C++
// End:
