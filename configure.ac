dnl Process this file with autoconf to produce a configure script.

AC_INIT([gpicview],[0.2.5],[],[gpicview])
# Autotols programs
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign no-dist-gzip dist-xz])
AM_MAINTAINER_MODE

AC_LANG([C])
LT_INIT

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_HEADER_STDC

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE(more_warnings,
       [AS_HELP_STRING([--enable-more-warnings],[Add more warnings @<:@default=no@:>@])],
       [enable_more_warnings="${enableval}"],
       [enable_more_warnings=no]
)

if test x"$enable_more_warnings" = x"yes"; then
  ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=implicit-int -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2 -DG_DISABLE_DEPRECATED -DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"
fi
AC_SUBST(ADDITIONAL_FLAGS)


AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.40.0])

AC_ARG_ENABLE(gtk3,
AS_HELP_STRING([--enable-gtk3],[enable to use gtk-3.0 instead of gtk-2.0]),
[case "${enableval}" in
  yes)  enable_gtk3=yes ;;
  no)   enable_gtk3=no ;;
  *) AC_MSG_ERROR([bad value "${enableval}" for --enable-gtk3, use "yes" (default) or "no".]) ;;
esac],[])

if test "x$enable_gtk3" = "xyes" ; then
  CFLAGS="$CFLAGS -DENABLE_GTK3"
  gtk_modules="gtk+-3.0 >= 3.0.0"
else
  gtk_modules="gtk+-2.0 >= 2.12.0"
fi
PKG_CHECK_MODULES(GTK, [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

pkg_modules="x11"

PKG_CHECK_MODULES(X11, [$pkg_modules])
AC_CHECK_LIB(jpeg, jpeg_start_decompress, [],
	     [AC_MSG_ERROR([libjpeg not found])], -lm)
AC_CHECK_HEADER(jpeglib.h, , [AC_MSG_ERROR([libjpeg not found])])
LIBJPEG='-ljpeg'

AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

# Generate po/LINGUAS on the fly rather than relying on translators
# to maintain it manually. This also overcome the problem that Transifex
# cannot add a language to po/LINGUAS if a new po file is submitted.
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

GETTEXT_PACKAGE=gpicview
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
])
AC_OUTPUT

echo
echo GPicView....................... : Version $VERSION
echo
echo Prefix..........................: $prefix
echo
echo The binary will be installed in $prefix/bin
echo
echo http://lxde.org/
echo
