/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2008,2009 Jos De Laender <jos.de_laender@telenet.be>
** Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef DLDEFINES_H
#define DLDEFINES_H

// pulls in INT32_MAX etc.
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <memory>

// disable the file manager
// #define PT_WITHOUT_FILEMGR


#pragma clang diagnostic warning  "-Wabi"
#pragma clang diagnostic warning  "-Waddress-of-temporary"
#pragma clang diagnostic warning  "-Waddress"
#pragma clang diagnostic warning  "-Waggregate-return"
#pragma clang diagnostic warning  "-Wall"
#pragma clang diagnostic warning  "-Wambiguous-member-template"
#pragma clang diagnostic warning  "-Warc-abi"
#pragma clang diagnostic warning  "-Warc-non-pod-memaccess"
#pragma clang diagnostic warning  "-Warc-retain-cycles"
#pragma clang diagnostic warning  "-Warc-unsafe-retained-assign"
#pragma clang diagnostic warning  "-Warc"
#pragma clang diagnostic warning  "-Watomic-properties"
#pragma clang diagnostic ignored "-Wattributes"
#pragma clang diagnostic warning  "-Wavailability"
#pragma clang diagnostic warning  "-Wbad-function-cast"
#pragma clang diagnostic warning  "-Wbind-to-temporary-copy"
#pragma clang diagnostic warning  "-Wbitwise-op-parentheses"
#pragma clang diagnostic warning  "-Wbool-conversions"
#pragma clang diagnostic warning  "-Wbuiltin-macro-redefined"
#pragma clang diagnostic warning  "-Wc++-compat"
#pragma clang diagnostic warning  "-Wc++0x-compat"
#pragma clang diagnostic warning  "-Wc++0x-extensions"
#pragma clang diagnostic warning  "-Wcast-align"
#pragma clang diagnostic warning  "-Wcast-qual"
#pragma clang diagnostic warning  "-Wchar-align"
#pragma clang diagnostic warning  "-Wchar-subscripts"
#pragma clang diagnostic warning  "-Wcomment"
#pragma clang diagnostic warning  "-Wcomments"
#pragma clang diagnostic warning  "-Wconditional-uninitialized"
#pragma clang diagnostic warning  "-Wconversion"
#pragma clang diagnostic warning  "-Wctor-dtor-privacy"
#pragma clang diagnostic warning  "-Wcustom-atomic-properties"
#pragma clang diagnostic warning  "-Wdeclaration-after-statement"
#pragma clang diagnostic warning  "-Wdefault-arg-special-member"
#pragma clang diagnostic warning  "-Wdelegating-ctor-cycles"
#pragma clang diagnostic warning  "-Wdelete-non-virtual-dtor"
#pragma clang diagnostic warning  "-Wdeprecated-declarations"
#pragma clang diagnostic warning  "-Wdeprecated-implementations"
#pragma clang diagnostic warning  "-Wdeprecated-writable-strings"
#pragma clang diagnostic warning  "-Wdeprecated"
#pragma clang diagnostic warning  "-Wdisabled-optimization"
#pragma clang diagnostic warning  "-Wdiscard-qual"
#pragma clang diagnostic warning  "-Wdiv-by-zero"
#pragma clang diagnostic warning  "-Wduplicate-method-arg"
#pragma clang diagnostic warning  "-Weffc++"
#pragma clang diagnostic warning  "-Wempty-body"
#pragma clang diagnostic warning  "-Wendif-labels"
#pragma clang diagnostic warning  "-Wexit-time-destructors"
#pragma clang diagnostic warning  "-Wextra-tokens"
#pragma clang diagnostic warning  "-Wextra"
#pragma clang diagnostic warning  "-Wformat-extra-args"
#pragma clang diagnostic warning  "-Wformat-nonliteral"
#pragma clang diagnostic warning  "-Wformat-zero-length"
#pragma clang diagnostic warning  "-Wformat"
#pragma clang diagnostic warning  "-Wformat=2"
#pragma clang diagnostic warning  "-Wfour-char-constants"
#pragma clang diagnostic warning  "-Wglobal-constructors"
#pragma clang diagnostic warning  "-Wgnu-designator"
#pragma clang diagnostic warning  "-Wgnu"
#pragma clang diagnostic warning  "-Wheader-hygiene"
#pragma clang diagnostic warning  "-Widiomatic-parentheses"
#pragma clang diagnostic warning  "-Wignored-qualifiers"
#pragma clang diagnostic warning  "-Wimplicit-atomic-properties"
#pragma clang diagnostic warning  "-Wimplicit-function-declaration"
#pragma clang diagnostic warning  "-Wimplicit-int"
#pragma clang diagnostic warning  "-Wimplicit"
#pragma clang diagnostic warning  "-Wimport"
#pragma clang diagnostic warning  "-Wincompatible-pointer-types"
#pragma clang diagnostic warning  "-Winit-self"
#pragma clang diagnostic warning  "-Winitializer-overrides"
#pragma clang diagnostic warning  "-Winline"
#pragma clang diagnostic warning  "-Wint-to-pointer-cast"
#pragma clang diagnostic warning  "-Winvalid-offsetof"
#pragma clang diagnostic warning  "-Winvalid-pch"
#pragma clang diagnostic warning  "-Wlarge-by-value-copy"
#pragma clang diagnostic warning  "-Wliteral-range"
#pragma clang diagnostic warning  "-Wlocal-type-template-args"
#pragma clang diagnostic warning  "-Wlogical-op-parentheses"
#pragma clang diagnostic warning  "-Wlong-long"
#pragma clang diagnostic warning  "-Wmain"
#pragma clang diagnostic warning  "-Wmicrosoft"
#pragma clang diagnostic warning  "-Wmismatched-tags"
#pragma clang diagnostic warning  "-Wmissing-braces"
#pragma clang diagnostic warning  "-Wmissing-declarations"
#pragma clang diagnostic warning  "-Wmissing-field-initializers"
#pragma clang diagnostic warning  "-Wmissing-format-attribute"
#pragma clang diagnostic warning  "-Wmissing-include-dirs"
#pragma clang diagnostic warning  "-Wmissing-noreturn"
#pragma clang diagnostic warning  "-Wmost"
#pragma clang diagnostic warning  "-Wmultichar"
#pragma clang diagnostic warning  "-Wnested-externs"
#pragma clang diagnostic warning  "-Wnewline-eof"
#pragma clang diagnostic warning  "-Wnon-gcc"
#pragma clang diagnostic warning  "-Wnon-virtual-dtor"
#pragma clang diagnostic warning  "-Wnonnull"
#pragma clang diagnostic warning  "-Wnonportable-cfstrings"
#pragma clang diagnostic warning  "-Wnull-dereference"
#pragma clang diagnostic warning  "-Wobjc-nonunified-exceptions"
#pragma clang diagnostic warning  "-Wold-style-cast"
#pragma clang diagnostic warning  "-Wold-style-definition"
#pragma clang diagnostic warning  "-Wout-of-line-declaration"
#pragma clang diagnostic warning  "-Woverflow"
#pragma clang diagnostic warning  "-Woverlength-strings"
#pragma clang diagnostic warning  "-Woverloaded-virtual"
#pragma clang diagnostic warning  "-Wpacked"
#pragma clang diagnostic warning  "-Wpadded"
#pragma clang diagnostic warning  "-Wparentheses"
#pragma clang diagnostic warning  "-Wpointer-arith"
#pragma clang diagnostic warning  "-Wpointer-to-int-cast"
#pragma clang diagnostic warning  "-Wprotocol"
#pragma clang diagnostic warning  "-Wreadonly-setter-attrs"
#pragma clang diagnostic warning  "-Wredundant-decls"
#pragma clang diagnostic warning  "-Wreorder"
#pragma clang diagnostic warning  "-Wreturn-type"
#pragma clang diagnostic warning  "-Wself-assign"
#pragma clang diagnostic warning  "-Wsemicolon-before-method-body"
#pragma clang diagnostic warning  "-Wsequence-point"
#pragma clang diagnostic warning  "-Wshadow"
#pragma clang diagnostic warning  "-Wshorten-64-to-32"
#pragma clang diagnostic warning  "-Wsign-compare"
#pragma clang diagnostic warning  "-Wsign-promo"
#pragma clang diagnostic warning  "-Wsizeof-array-argument"
#pragma clang diagnostic warning  "-Wstack-protector"
#pragma clang diagnostic warning  "-Wstrict-aliasing"
#pragma clang diagnostic warning  "-Wstrict-overflow"
#pragma clang diagnostic warning  "-Wstrict-prototypes"
#pragma clang diagnostic warning  "-Wstrict-selector-match"
#pragma clang diagnostic warning  "-Wsuper-class-method-mismatch"
#pragma clang diagnostic warning  "-Wswitch-default"
#pragma clang diagnostic warning  "-Wswitch-enum"
#pragma clang diagnostic warning  "-Wswitch"
#pragma clang diagnostic warning  "-Wsynth"
#pragma clang diagnostic warning  "-Wtautological-compare"
#pragma clang diagnostic warning  "-Wtrigraphs"
#pragma clang diagnostic warning  "-Wtype-limits"
#pragma clang diagnostic warning  "-Wundeclared-selector"
#pragma clang diagnostic warning  "-Wuninitialized"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic warning  "-Wunnamed-type-template-args"
#pragma clang diagnostic warning  "-Wunneeded-internal-declaration"
#pragma clang diagnostic warning  "-Wunneeded-member-function"
#pragma clang diagnostic warning  "-Wunused-argument"
#pragma clang diagnostic warning  "-Wunused-exception-parameter"
#pragma clang diagnostic warning  "-Wunused-function"
#pragma clang diagnostic warning  "-Wunused-label"
#pragma clang diagnostic warning  "-Wunused-member-function"
#pragma clang diagnostic warning  "-Wunused-parameter"
#pragma clang diagnostic warning  "-Wunused-value"
#pragma clang diagnostic warning  "-Wunused-variable"
#pragma clang diagnostic warning  "-Wunused"
#pragma clang diagnostic warning  "-Wused-but-marked-unused"
#pragma clang diagnostic warning  "-Wvariadic-macros"
#pragma clang diagnostic warning  "-Wvector-conversions"
#pragma clang diagnostic warning  "-Wvla"
#pragma clang diagnostic warning  "-Wvolatile-register-var"
#pragma clang diagnostic warning  "-Wwrite-strings"


//==============================================================================

template<class T>
inline void DelAndNull(T*& p) {
  delete p;
  p = nullptr;
}

//==============================================================================
// Calculate the square
template <typename T>
inline T ptSqr(const T a) { return a*a; }

//==============================================================================

// Custom unique ptr creation. Because there is no std::make_unique().
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)... ));
}

//==============================================================================

/*! Returns `true` if `AValue` lies between `ALowBound` and `AHighBound`, `false` otherwise.
    Lower boundary is included in the range, higher boundary is not. */
template<typename T>
inline bool isBetween(const T &AValue, const T &ALowBound, const T &AHighBound) {
  return AValue >= ALowBound && AValue < AHighBound;
}

//==============================================================================

// Some macro's (most cannot go efficiently in functions).

#ifndef SQR
  #define SQR(x) ((x)*(x))
#endif
#undef  ABS
#undef  MIN
#undef  MAX
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }
#define SIGN(x) ((x) == 0 ? 0 : ((x) < 0 ? -1 : 1 ))
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __PRETTY_FUNCTION__
//":" __FILE__ ":" TOSTRING(__LINE__)
#define CRLF "\r\n"

// Thanks QT
template <typename T>
inline const T &ptMin(const T &a, const T &b) { if (a < b) return a; return b; }
template <typename T>
inline const T &ptMax(const T &a, const T &b) { if (a < b) return b; return a; }
template <typename T>
inline const T &ptBound(const T &min, const T &val, const T &max)
{ return ptMax(min, ptMin(max, val)); }

#define MIN(a, b)         ptMin(a, b)
#define MAX(a, b)         ptMax(a, b)
#define LIM(x, min, max)  ptBound(min, x, max)
#define ULIM(x, y, z)     ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x)           ptBound(0, x, 0xffff)

////////////////////////////////////////////////////////////////////////////////
//
// Following 'functions' are macro implemented for :
//   - configurability to get them out and in.
//   - to get always a correct reference to the line number where inserted.
//
////////////////////////////////////////////////////////////////////////////////

// Uncomment me for malloc/calloc/free related leaks.
// #define DEBUG_MEMORY

#ifdef DEBUG_MEMORY

  #include "ptCalloc.h"

  #define CALLOC(Num,Size)  ptCalloc(Num,Size,__FILE__,__LINE__,this)
  #define CALLOC2(Num,Size) ptCalloc(Num,Size,__FILE__,__LINE__,NULL)
  #define MALLOC(Size)      ptCalloc(1,Size,__FILE__,__LINE__,this)
  #define MALLOC2(Size)     ptCalloc(1,Size,__FILE__,__LINE__,NULL)
  #define FREE(x)           {ptFree(x,__FILE__,__LINE__,this); x=NULL;}
  #define FREE2(x)          {ptFree(x,__FILE__,__LINE__,NULL); x=NULL;}
  #define ALLOCATED(x)      ptAllocated(x,__FILE__,__LINE__)

#else

  #define CALLOC(Num,Size)  ptCalloc_Ex(Num,Size)
  #define CALLOC2(Num,Size) ptCalloc_Ex(Num,Size)
  #define MALLOC(Size) malloc(Size)
  #define MALLOC2(Size) malloc(Size)
  // Remark free(NULL) is valid nop !
  #define FREE(x) {free(x); x=NULL; }
  #define FREE2(x) {free(x); x=NULL; }
  #define ALLOCATED(x) ;

#endif

// Close a file descriptor and put it on NULL

#define FCLOSE(x) {                                                \
  if (x) fclose(x);                                                \
  x=NULL;                                                          \
};

// A function to trace key values in the program (debug of course)

#define ENABLE_TRACE
#ifdef ENABLE_TRACE
#define TRACEKEYVALS(x,y,z) {                                      \
  printf("(%-25s,%5d): ",__FILE__,__LINE__);                       \
  printf("%-20s: ",x);                                              \
  printf(y,z);                                                     \
  printf("\n");                                                    \
  fflush(stdout);                                                  \
}
#else
#define TRACEKEYVALS(x,y,z) ;
#endif

// TRACEMAIN does something comparably , but basically gives
// only progress info on the graphical pipe.

#define ENABLE_TRACEMAIN
#ifdef ENABLE_TRACEMAIN
#define TRACEMAIN(x,y) {                                           \
  printf("(%-25s,%5d): ",__FILE__,__LINE__);                       \
  printf(x,y);                                                     \
  printf("\n");                                                    \
  fflush(stdout);                                                  \
}
#else
#define TRACEMAIN(x,y) ;
#endif

// Debug function can be used to drop quick and dirty an image.
// w,h are the dimensions. b is the buffer of the image.
// depth is 3 or 4 (according to buffer layout).

#define DROP_IMAGE(w,h,b,depth) {                                  \
  char FileName[100];                                              \
  sprintf(FileName,"%s_%d.imagedrop",__FILE__,__LINE__);           \
  FILE *DropFile = fopen(FileName,"wb");                           \
  if (!DropFile) exit(EXIT_FAILURE);                               \
  if (fwrite(b,2*depth,w*h,DropFile) != w*h) exit(EXIT_FAILURE);   \
  fclose(DropFile);                                                \
}

#endif

////////////////////////////////////////////////////////////////////////////////
