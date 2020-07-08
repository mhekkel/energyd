AC_INIT([energyd], 1.0, [maarten@hekkelman.com])

dnl Switch to a C++ compiler, and check if it works.
AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_17([noext])

AC_CONFIG_SRCDIR([src/energyd.cpp])
dnl AC_CONFIG_HEADERS([src/ibs-config.hpp])
AC_CONFIG_AUX_DIR(config)
dnl AC_CONFIG_MACRO_DIR([config/m4])

AC_PREFIX_DEFAULT(/usr/local)

AC_PROG_INSTALL

AC_ARG_VAR([DEBUG], [Build a debug version of the application])
AC_ARG_VAR([MRC], [Specify a location for the mrc executable])

dnl We really want to use mrc
if test "x$MRC" = "x"; then
	AC_PATH_PROG([MRC], [mrc])
fi

if test "x$MRC" = "x"; then
	AC_MSG_ERROR([mrc not found, the application can not be built])
fi

AC_CHECK_PROG([HAVE_YARN], [yarn], [yes],
	[AC_MSG_ERROR([yarn was not found, it is required to package the javascripts.])])

AC_PATH_PROG([PKG_CONFIG], [pkg-config])

AX_PKG_CHECK_MODULES([PQXX], [libpq libpqxx], [], [], [AC_MSG_ERROR([the required package libpqxx-dev is not installed])])
AX_PKG_CHECK_MODULES([JPEG], [libjpeg], [], [], [AC_MSG_ERROR([the required package libjpeg-dev is not installed])])
AX_PKG_CHECK_MODULES([PNG], [libpng], [], [], [AC_MSG_ERROR([the required package libpng-dev is not installed])])
AX_PKG_CHECK_MODULES([TIDY], [tidy], [], [], [AC_MSG_ERROR([the required package libtidy-dev is not installed])])
AX_PKG_CHECK_MODULES([ZEEP], [libzeep-http], [], [], [AC_MSG_ERROR([the required package libzeep-dev is not installed])])

AX_PTHREAD(
	[
		LIBS="$PTHREAD_LIBS $LIBS"
		CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
	]
)

AC_CHECK_HEADER([filesystem], [], [AC_MSG_ERROR([The file <filesystem> is missing, perhaps you should install a more recent libstdc++ implementation.])])

dnl check if we need stdc++fs as library
AC_TRY_LINK(
	[#include <filesystem>],
	[(void)std::filesystem::current_path();],
	[],
	[
		LIBS="$LIBS -lstdc++fs"

		AC_TRY_LINK(
			[#include <filesystem>],
			[(void)std::filesystem::current_path();],
			[],
			[
				AC_MSG_ERROR([Could not link filesystem])
			]
		)
	]
)
AX_BOOST_BASE([1.65.1], [], [AC_MSG_ERROR([Could not find a recent version of boost])])
AX_BOOST_ASIO
AX_BOOST_PROGRAM_OPTIONS

dnl Process Makefile.in to create Makefile
AC_OUTPUT([GNUmakefile])
