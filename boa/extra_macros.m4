dnl Many of the following macros courtesy of:
dnl http://www.gnu.org/software/ac-archive/
dnl
dnl The Following macros courtesy of:
dnl Installed_Packages/check_gnu_make.html
dnl 
dnl Copyrights are by the individual authors, as listed.
dnl License: GPL
dnl


dnl CHECK_GNU_MAKE()
dnl
dnl This macro searches for a GNU version of make.  If a match is found, the
dnl makefile variable `ifGNUmake' is set to the empty string, otherwise it is
dnl set to "#".  This is useful for  including a special features in a Makefile,
dnl which cannot be handled by other versions of make.  The variable
dnl _cv_gnu_make_command is set to the command to invoke GNU make if it exists,
dnl the empty string otherwise.
dnl
dnl Here is an example of its use:
dnl
dnl Makefile.in might contain:
dnl
dnl     # A failsafe way of putting a dependency rule into a makefile
dnl     $(DEPEND):
dnl             $(CC) -MM $(srcdir)/*.c > $(DEPEND)
dnl
dnl     @ifGNUmake@ ifeq ($(DEPEND),$(wildcard $(DEPEND)))
dnl     @ifGNUmake@ include $(DEPEND)
dnl     @ifGNUmake@ endif
dnl
dnl Then configure.in would normally contain:
dnl
dnl     CHECK_GNU_MAKE()
dnl     AC_OUTPUT(Makefile)
dnl
dnl Then perhaps to cause gnu make to override any other make, we could do
dnl something like this (note that GNU make always looks for GNUmakefile first):
dnl
dnl     if  ! test x$_cv_gnu_make_command = x ; then
dnl             mv Makefile GNUmakefile
dnl             echo .DEFAULT: > Makefile ;
dnl             echo \  $_cv_gnu_make_command \$@ >> Makefile;
dnl     fi
dnl
dnl Then, if any (well almost any) other make is called, and GNU make also exists,
dnl then the other make wraps the GNU make.
dnl
dnl John Darrington <j.darrington@elvis.murdoch.edu.au>
dnl 1.3 (2002/01/04)
dnl
dnl Modified 18 Sep 2002 by Jon Nelson <jnelson@boa.org>

AC_DEFUN([CHECK_GNU_MAKE], 
         [ AC_CACHE_CHECK( for GNU make, _cv_gnu_make_command,
           [_cv_gnu_make_command=''
           dnl Search all the common names for GNU make
           for a in "$MAKE" make gmake gnumake ; do
             if test -z "$a" ; then continue ; fi
             if ( sh -c "$a --version" 2> /dev/null | grep GNU  2>&1 > /dev/null ); then
               _cv_gnu_make_command=$a
               break;
             fi
           done
           if test "x$_cv_gnu_make_command" = "x"; then
             _cv_gnu_make_command="Not found"
           fi
           ])
dnl If there was a GNU version, then set @ifGNUmake@ to the empty string, '#' otherwise
         if test "$_cv_gnu_make_command" != "Not found"; then
           ifGNUmake='';
         else
           ifGNUmake='#';
           _cv_gnu_make_command='';
         fi
         AC_SUBST(ifGNUmake)
        ])

dnl AC_CHECK_STRUCT_FOR(INCLUDES,STRUCT,MEMBER,DEFINE,[no]) 
dnl 1.1 (2000/09/19) 
dnl Wes Hardaker <wjhardaker@ucdavis.edu> 

dnl ----------------------------------------------------------

AC_DEFUN([AC_CHECK_STRUCT_FOR],[
ac_safe_struct=`echo "$2" | sed 'y%./+-%__p_%'`
ac_safe_member=`echo "$3" | sed 'y%./+-%__p_%'`
ac_safe_all="ac_cv_struct_${ac_safe_struct}_has_${ac_safe_member}"
changequote(, )dnl
  ac_uc_define=STRUCT_`echo "${ac_safe_struct}_HAS_${ac_safe_member}" | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl

AC_MSG_CHECKING([for $2.$3])
AC_CACHE_VAL($ac_safe_all,
[
if test "x$4" = "x"; then
  defineit="= 0"
elif test "x$4" = "xno"; then
  defineit=""
else
  defineit="$4"
fi
AC_TRY_COMPILE([
$1
],[
struct $2 testit;
testit.$3 $defineit;
], eval "${ac_safe_all}=yes", eval "${ac_safe_all}=no" )
])

if eval "test \"x$`echo ${ac_safe_all}`\" = \"xyes\""; then
  AC_MSG_RESULT(yes)
  AC_DEFINE_UNQUOTED($ac_uc_define)
else
  AC_MSG_RESULT(no)
fi
])

dnl @synopsis AC_C_VAR_FUNC
dnl
dnl This macro tests if the C complier supports the C9X standard
dnl __func__ indentifier.
dnl
dnl The new C9X standard for the C language stipulates that the
dnl identifier __func__ shall be implictly declared by the compiler
dnl as if, immediately following the opening brace of each function
dnl definition, the declaration
dnl
dnl     static const char __func__[] = "function-name";
dnl
dnl appeared, where function-name is the name of the function where
dnl the __func__ identifier is used.
dnl
dnl @author Christopher Currie <christopher@currie.com>

AC_DEFUN([AC_C_VAR_FUNC],
[AC_REQUIRE([AC_PROG_CC])
AC_CACHE_CHECK(whether $CC recognizes __func__, ac_cv_c_var_func,
AC_TRY_COMPILE(,
[int main() {
char *s = __func__;
}],
AC_DEFINE(HAVE_FUNC,,
[Define if the C complier supports __func__]) ac_cv_c_var_func=yes,
ac_cv_c_var_func=no) )
])dnl

dnl Exports one of ac_cv_func_poll or ac_cv_func_select
dnl Author - Jon Nelson <jnelson@boa.org> 
dnl Copyright 2002
AC_DEFUN([POLL_OR_SELECT],
  [
    AC_MSG_CHECKING(whether to use poll or select)
    AC_ARG_WITH(poll,
    [  --with-poll             Use poll],
    [
      if test "$withval" = "yes" ; then
        AC_MSG_RESULT(trying poll)
        ac_x=1
      else
        AC_MSG_RESULT(trying select)
        ac_x=0
      fi
    ],
    [
      AC_MSG_RESULT(trying select)
      ac_x=0
    ])

    if test $ac_x = 1; then
      AC_CHECK_HEADERS(sys/poll.h)
      AC_CHECK_FUNCS(poll)
      if test "x$ac_cv_func_poll" = "x"; then
        AC_MSG_ERROR(We attempted to find poll but could not. Please try again with --without-poll)
      fi
      BOA_ASYNC_IO="poll"
    else
      AC_CHECK_HEADERS(sys/select.h)
      AC_CHECK_FUNCS(select)
      if test "x$ac_cv_func_select" = "x"; then
        AC_MSG_ERROR(We attempted to find select but could not. Please try again with --with-poll)
      fi
      BOA_ASYNC_IO="select"
    fi
  ]
)

