#pragma once

#include <QtGlobal>

#if defined(NESCORELIB_LIBRARY)
#  define NESCORELIB_EXPORT Q_DECL_EXPORT
#else
#  define NESCORELIB_EXPORT Q_DECL_IMPORT
#endif
