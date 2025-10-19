/* stddef.h — minimal, C-only, for a freestanding kernel */
#ifndef KERNEL_STDDEF_H
#define KERNEL_STDDEF_H

/* NULL */
#define NULL ((void*)0)

/* size_t */
#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;       /* tip exact oferit de compilator */
#else
typedef unsigned int size_t;        /* fallback (bun pe i386) */
#endif

/* ptrdiff_t */
#if defined(__PTRDIFF_TYPE__)
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#else
typedef int ptrdiff_t;              /* fallback (bun pe i386) */
#endif

/* wchar_t (în C nu e keyword, deci îl definim) */
#if !defined(__WCHAR_TYPE__)
typedef unsigned short wchar_t;     /* fallback rezonabil pe i386 */
#else
typedef __WCHAR_TYPE__ wchar_t;
#endif

/* offsetof */
#define offsetof(type, member) ((size_t)((char*)&(((type*)0)->member) - (char*)((type*)0)))

#endif /* KERNEL_STDDEF_H */


//source: ChatGPT 5.0