dnl $Id$
dnl config.m4 for extension vstats

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(vstats, for vstats support,
dnl Make sure that the comment is aligned:
dnl [  --with-vstats             Include vstats support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(vstats, whether to enable vstats support,
dnl Make sure that the comment is aligned:
[  --enable-vstats           Enable vstats support])

if test "$PHP_VSTATS" != "no"; then

  PHP_NEW_EXTENSION(vstats, vstats.c, $ext_shared)
  PHP_SUBST(VSTATS_SHARED_LIBADD)
  
  dnl # --with-vstats -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/vstats.h"  # you most likely want to change this
  dnl if test -r $PHP_VSTATS/$SEARCH_FOR; then # path given as parameter
  dnl   VSTATS_DIR=$PHP_VSTATS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for vstats files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       VSTATS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$VSTATS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the vstats distribution])
  dnl fi

  dnl # --with-vstats -> add include path
  dnl PHP_ADD_INCLUDE($VSTATS_DIR/include)

  dnl # --with-vstats -> check for lib and symbol presence
  dnl LIBNAME=vstats # you may want to change this
  dnl LIBSYMBOL=vstats # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $VSTATS_DIR/lib, VSTATS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_VSTATSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong vstats lib version or lib not found])
  dnl ],[
  dnl   -L$VSTATS_DIR/lib -lm -ldl
  dnl ])

  dnl Find library
  AC_MSG_CHECKING(for uuid in default path)
  for i in /usr/local /usr; do
    if test -r $i/lib64/libuuid.a; then
      UUID_DIR=$i
      UUID_LIB_DIR="lib64"
      AC_MSG_RESULT(found in $i)
    elif test -r $i/lib/libuuid.a; then
      UUID_DIR=$i
      UUID_LIB_DIR="lib"
      AC_MSG_RESULT(found in $i)
    fi
  done
  
  dnl Check library is what we expect before adding it
  PHP_CHECK_LIBRARY(uuid, uuid_generate,
  [
    PHP_ADD_LIBRARY_WITH_PATH(uuid, $UUID_DIR/$UUID_LIB_DIR, VSTATS_SHARED_LIBADD)
  ],[
    AC_MSG_ERROR([Invalid uuid lib, uuid_generate() not found])
  ],[
  ])  

fi
