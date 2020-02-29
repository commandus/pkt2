AC_PREREQ([2.63])
AC_INIT([pkt2], [0.1], [andrei.i.ivanov@commandus.com])
AM_INIT_AUTOMAKE([1.11 foreign no-define subdir-objects no-dependencies])
AC_CONFIG_FILES([Makefile tests/Makefile])
AC_CONFIG_SRCDIR([pkt2receiver.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
	[enable debugging, default: no]),
[case "${enableval}" in
	yes) debug=true ;;
	no)  debug=false ;;
	*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# Header only https://github.com/tplgy/cppcodec
CPPFLAGS="$CPPFLAGS -Icppcodec"

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h string.h sys/time.h termios.h unistd.h])
AC_CHECK_HEADERS([argtable2.h], [], AC_MSG_ERROR([Could not find argtable2.h. Try $ ./configure CFLAGS='-Iyour-argtable2-include-path]))

AC_CHECK_HEADERS([curl/curl.h], [], AC_MSG_ERROR([Could not find curl/curl.h. Try $ ./configure CFLAGS='-Iyour-curl-include-path]))
AC_CHECK_LIB(curl, curl_easy_init, [], AC_MSG_ERROR([Could not find libcurl. Try $ ./configure LDFLAGS='-Lyour-curl-lib-path']))


if test x$curl = xfalse
then
	AM_CONDITIONAL(HAVE_LIBCURL, false)
if test "$gnurl" = 0
then
	AC_MSG_WARN([GNUnet requires libcurl-gnutls or gnurl >= 7.34])
fi
else
	AM_CONDITIONAL(HAVE_LIBCURL, true)
	AC_DEFINE([HAVE_LIBCURL],[1],[Have CURL])
fi


# postresgql header
AC_CHECK_HEADERS(libpq-fe.h)
if test "$ac_cv_header_libpq_fe_h" = "yes"; then
  POSTGRES=yes
else
  old="$CPPFLAGS"
  CPPFLAGS="$CPPFLAGS -I/usr/include/postgresql"
  AC_CHECK_HEADERS(/usr/include/postgresql/libpq-fe.h)
  if test "$ac_cv_header__usr_include_postgresql_libpq_fe_h" = "yes"; then
    POSTGRES=yes
  else
    CPPFLAGS="$CPPFLAGS -I/usr/include/pgsql"
    AC_CHECK_HEADERS(/usr/include/pgsql/libpq-fe.h)
    if test "$ac_cv_header__usr_include_pgsql_libpq_fe_h" = "yes"; then
      POSTGRES=yes
    else
      CPPFLAGS="$CPPFLAGS -I/usr/local/include/pgsql"
      AC_CHECK_HEADERS(/usr/local/include/pgsql/libpq-fe.h)
      if test "$ac_cv_header__usr_local_include_pgsql_libpq_fe_h" = "yes"; then
        POSTGRES=yes
      else
        CPPFLAGS="$CPPFLAGS -I/usr/pgsql-9.4/include"
        AC_CHECK_HEADERS(/usr/pgsql-9.4/include/libpq-fe.h)
        if test "$ac_cv_header__usr_pgsql_9_4_include_libpq_fe_h" = "yes"; then
          POSTGRES=yes
        else
          CPPFLAGS="$CPPFLAGS -I/usr/pgsql-9.5/include"
          AC_CHECK_HEADERS(/usr/pgsql-9.5/include/libpq-fe.h)
          if test "$ac_cv_header__usr_pgsql_9_5_include_libpq_fe_h" = "yes"; then
            POSTGRES=yes
          else
            CPPFLAGS="$save_CPPFLAGS"
            POSTGRES=
            echo "**************************************************************"
            echo "Unable to locate libpq (postgres) headers (are they installed)"
            echo "**************************************************************"
          fi
        fi
      fi
    fi
  fi
fi

# postgresql library
if test "$POSTGRES" = "yes"; then
  if test -d /usr/pgsql-9.4/lib ; then
    CPPFLAGS="$CPPFLAGS -L/usr/pgsql-9.4/lib"
    LDFLAGS="$LDFLAGS -L/usr/pgsql-9.4/lib"
  else
    if test -d /usr/lib/pgsql ; then
      CPPFLAGS="$CPPFLAGS -L/usr/lib/pgsql"
      LDFLAGS="$LDFLAGS -L/usr/lib/pgsql"
    else
      if test -d /usr/local/lib/pgsql ; then
        CPPFLAGS="$CPPFLAGS -L/usr/local/lib/pgsql"
        LDFLAGS="$LDFLAGS -L/usr/local/lib/pgsql"
      fi
    fi
  fi
  AC_CHECK_LIB(pq,PQfformat)
  if test "$ac_cv_lib_pq_PQfformat" != "yes"; then
      POSTGRES=
      AC_CHECK_LIB(pq,PQclear)
      echo "******************************************************"
      if test "$ac_cv_lib_pq_PQclear" != "yes"; then
        echo "Unable to locate postgres pq library (is it installed)"
      else
        echo "Located postgres pq library, but it is too old to use!"
      fi
      echo "Perhaps you can try 'configure --with-postgres=dir=path'"
      echo "to point to the postgres version you wish to use."
      echo "******************************************************"
  fi
fi

AC_CHECK_LIB(argtable2, arg_parse, [], AC_MSG_ERROR([Could not find libargtable2. Try $ ./configure LDFLAGS='-Lyour-argtable2-lib-path']))

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([memmove memset])

AC_OUTPUT
