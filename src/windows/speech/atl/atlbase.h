/*
    A small stand-in for the parts of Microsoft's ATL that the speech code
    (SapiInterface) and the Windows SDK's sphelper.h use. It is not ATL.

    Windows.dll is built on a Mac with clang-cl and the Windows SDK from xwin,
    which doesn't include ATL. CMake adds this folder only then
    (QUORUM_ATL_STANDIN, set by toolchains/clang-cl-xwin.cmake). A Visual
    Studio build on Windows uses the real ATL instead.
*/
#pragma once
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>

#ifndef ATLASSERT
#define ATLASSERT(x) ((void)0)
#endif

// A COM pointer that calls AddRef and Release, like ATL's CComPtr.
template <class T> class CComPtr {
public:
    T* p = nullptr;

    CComPtr() noexcept {}
    CComPtr(T* other) noexcept : p(other) { if (p) p->AddRef(); }
    CComPtr(const CComPtr& other) noexcept : p(other.p) { if (p) p->AddRef(); }
    ~CComPtr() { Release(); }

    void Release() noexcept
    {
        if (p)
        {
            T* old = p;
            p = nullptr;
            old->Release();
        }
    }

    CComPtr& operator=(T* other) noexcept
    {
        if (other) other->AddRef();
        Release();
        p = other;
        return *this;
    }
    CComPtr& operator=(const CComPtr& other) noexcept { return *this = other.p; }

    operator T*() const noexcept { return p; }
    T* operator->() const noexcept { return p; }
    // ATL asserts the pointer is empty here; callers pass it to functions that fill it in.
    T** operator&() noexcept { return &p; }
    bool operator!() const noexcept { return p == nullptr; }
    bool operator==(T* other) const noexcept { return p == other; }

    T* Detach() noexcept
    {
        T* old = p;
        p = nullptr;
        return old;
    }

    void Attach(T* other) noexcept
    {
        Release();
        p = other;
    }

    HRESULT CopyTo(T** out) noexcept
    {
        if (out == nullptr) return E_POINTER;
        *out = p;
        if (p) p->AddRef();
        return S_OK;
    }

    template <class Q> HRESULT QueryInterface(Q** out) const noexcept
    {
        return p->QueryInterface(__uuidof(Q), (void**)out);
    }

    HRESULT CoCreateInstance(REFCLSID classId, IUnknown* outer = nullptr, DWORD context = CLSCTX_ALL) noexcept
    {
        return ::CoCreateInstance(classId, outer, context, __uuidof(T), (void**)&p);
    }
};

#define USES_CONVERSION
