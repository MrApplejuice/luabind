// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LUABIND_BACK_REFERENCE_040510_HPP
#define LUABIND_BACK_REFERENCE_040510_HPP

#include <luabind/lua_state_fwd.hpp>

#if !defined(LUABIND_NO_RTTI) && !defined(LUABIND_WRAPPER_BASE_HPP_INCLUDED)
# include <luabind/wrapper_base.hpp>
#endif

#include <luabind/detail/has_get_pointer.hpp>
#include <luabind/get_pointer.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/mpl/bool.hpp>

namespace luabind {
  struct wrap_base;

namespace detail
{
  namespace mpl = boost::mpl;
 
  template<class T>
  wrap_base const* get_back_reference_aux0(T const* p, mpl::true_)
  {
      return dynamic_cast<wrap_base const*>(p);
  }

  template<class T>
  wrap_base const* get_back_reference_aux0(T const*, mpl::false_)
  {
      return 0;
  }

  template<class T>
  wrap_base const* get_back_reference_aux1(T const* p)
  {
      return get_back_reference_aux0(p, boost::is_polymorphic<T>());
  }

  template<class T>
  wrap_base const* get_back_reference_aux2(T const& x, mpl::true_)
  {
      return get_back_reference_aux1(get_pointer(x));
  }

  template<class T>
  wrap_base const* get_back_reference_aux2(T const& x, mpl::false_)
  {
      return get_back_reference_aux1(&x);
  }

  template<class T>
  wrap_base const* get_back_reference(T const& x)
  {
      return detail::get_back_reference_aux2(
          x
        , has_get_pointer<T>()
      );
  }
  
} // namespace detail

/**
 * Returns the stored back reference for object x and pushes it on the
 * lua stack - if possible. This only works if x inherits from
 * wrap_base. If the value was not pushed succesfully, the function 
 * returns false.
 */
template<class T>
bool get_back_reference(lua_State* L, T const& x)
{
#ifndef LUABIND_NO_RTTI
    if (wrap_base const* w = detail::get_back_reference(x))
    {
        if (detail::wrap_access::ref(*w).is_weakref_valid()) 
        {
            detail::wrap_access::ref(*w).get(L);
            return true;
        }
    }
#endif
    return false;
}

template<class T>
bool move_back_reference(lua_State* L, T const& x)
{
#ifndef LUABIND_NO_RTTI
    if (wrap_base* w = const_cast<wrap_base*>(detail::get_back_reference(x)))
    {
        assert(detail::wrap_access::ref(*w).m_strong_ref.is_valid());
        detail::wrap_access::ref(*w).get(L);
        detail::wrap_access::ref(*w).m_strong_ref.reset();
        return true;
    }
#endif
    return false;
}

/**
 * Tries to add a back reference for the given object. An initialized 
 * reference must already be present at the lua stack position
 * given by the parameter index. This value will remain untouched, but
 * be set as the back reference for object x. This will only succeed
 * if x inherits from wrap_base.
 */
template<class T>
void try_add_back_reference(lua_State* L, T const& x, int index)
{
#ifndef LUABIND_NO_RTTI
    if (wrap_base* w = const_cast<wrap_base*>(detail::get_back_reference(x)))
    {
        if (index < 0) {
            index = lua_gettop(L) + 1 + index;
        }
        weak_ref tmpWeakRef(L, L, index);
        detail::wrap_access::ref(*w).swap(tmpWeakRef);
    }
#endif
}

} // namespace luabind

#endif // LUABIND_BACK_REFERENCE_040510_HPP

