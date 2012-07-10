dnl $Id$
dnl config.m4 for extension makuo

PHP_ARG_ENABLE(makuo, whether to enable makuo support,
Make sure that the comment is aligned:
[  --enable-makuo           Enable makuo support])

if test "$PHP_MAKUO" != "no"; then
  PHP_NEW_EXTENSION(makuo, makuo.c, $ext_shared)
fi
