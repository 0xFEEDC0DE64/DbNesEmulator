#pragma once

#include <QtGlobal>

#if defined(NESGUILIB_LIBRARY)
#  define NESGUILIB_EXPORT Q_DECL_EXPORT
#else
#  define NESGUILIB_EXPORT Q_DECL_IMPORT
#endif
