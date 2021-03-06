# Makefile.am for libgpg-error.
# Copyright (C) 2003, 2006, 2007, 2013 g10 Code GmbH
#
# This file is part of libgpg-error.
#
# libgpg-error is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# libgpg-error is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, see <http://www.gnu.org/licenses/>.

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-bzip2

# (A suitable gitlog-to-changelog script can be found in GnuPG master.)
GITLOG_TO_CHANGELOG=gitlog-to-changelog

EXTRA_DIST = autogen.sh config.rpath libgpg-error.spec.in 		\
             COPYING COPYING.LIB potomo doc/HACKING ChangeLog-2011   	\
	     po/ChangeLog-2011 m4/ChangeLog-2011 contrib/ChangeLog-2011 \
             build-aux/git-log-footer  build-aux/git-log-fix

if LANGUAGES_SOME
lang_subdirs = lang
else
lang_subdirs =
endif

SUBDIRS = m4 src tests po $(lang_subdirs)

# this is automake create am__relativize
am__old__relativize = \
  dir0=`pwd`; \
  sed_first='s,^\([^/]*\)/.*$$,\1,'; \
  sed_rest='s,^[^/]*/*,,'; \
  sed_last='s,^.*/\([^/]*\)$$,\1,'; \
  sed_butlast='s,/*[^/]*$$,,'; \
  while test -n "$$dir1"; do \
    first=`echo "$$dir1" | sed -e "$$sed_first"`; \
    if test "$$first" != "."; then \
      if test "$$first" = ".."; then \
        dir2=`echo "$$dir0" | sed -e "$$sed_last"`/"$$dir2"; \
        dir0=`echo "$$dir0" | sed -e "$$sed_butlast"`; \
      else \
        first2=`echo "$$dir2" | sed -e "$$sed_first"`; \
        if test "$$first2" = "$$first"; then \
          dir2=`echo "$$dir2" | sed -e "$$sed_rest"`; \
        else \
          dir2="../$$dir2"; \
        fi; \
        dir0="$$dir0"/"$$first"; \
      fi; \
    fi; \
    dir1=`echo "$$dir1" | sed -e "$$sed_rest"`; \
  done; \
  reldir="$$dir2"

convert_abs_to_rel_path = \
  conv_current_path=`pwd`; \
  conv_abs_path="$$conv_dist"; \
  conv_sed_is_abs='s,^/.*,ok,'; \
  conv_is_abs=`echo "$$conv_abs_path" | sed -e "$$conv_sed_is_abs"`; \
  if test "$$conv_is_abs" = "ok"; then \
	$(conv_print_is_abs); \
	$(conv_define_sed_variable); \
	$(conv_get_standard_dir_exp); \
	$(conv_get_same_path); \
	$(conv_get_standard_relative_dir_exp); \
	$(conv_generate_relative_dir); \
	$(conv_abs_in_current_path); \
	echo "conv_same_path=$$conv_same_path"; \
	echo "conv_rel_path=$$conv_rel_path"; \
        conv_dist="$$conv_rel_path"; \
  fi 

conv_print_is_abs = \
  echo "$$conv_abs_path is abs, convert start."

conv_define_sed_variable = \
  conv_sed_get_first='s,^\(//*[^/]*\).*,\1,'; \
  conv_sed_del_first='s,^\(//*[^/]*\),,'; \
  conv_sed_del_last='s,\(//*[^/]*\)$$,,'; \
  conv_sed_del_last_slash='s,\(//*\)$$,,'; \
  conv_sed_del_all_redu_slash='s,\(//*\),/,g'; \
  conv_sed_del_first_slash='s,^\(//*\),,'


#ensure there isn't last slash (/bin/ --> /bin)
#ensure there isn't redu slash (//bin --> /bin)
#ensure there isn't . or ..  (/usr/bin/../ --> /usr  /usr/bin/./ --> /usr/bin)
conv_get_standard_dir_exp = \
  conv_abs_path=`echo "$$conv_abs_path" | sed -e "$$conv_sed_del_last_slash"`; \
  conv_current_path=`echo "$$conv_current_path" | sed -e "$$conv_sed_del_last_slash"`; \
  conv_abs_path=`echo "$$conv_abs_path" | sed -e "$$conv_sed_del_all_redu_slash"`; \
  conv_current_path=`echo "$$conv_current_path" | sed -e "$$conv_sed_del_all_redu_slash"`; \
  conv_resolved_abs_path=""; \
  conv_tmp_abs_path="$$conv_abs_path"; \
  while test -n "$$conv_tmp_abs_path"; do \
    conv_first_abs_path=`echo "$$conv_tmp_abs_path" | sed -e "$$conv_sed_get_first"`; \
	conv_tmp_abs_path=`echo "$$conv_tmp_abs_path" | sed -e "$$conv_sed_del_first"`; \
	if test "$$conv_first_abs_path" = "/.."; then \
	  conv_resolved_abs_path=`echo "$$conv_resolved_abs_path" | sed -e "$$conv_sed_del_last"`; \
	elif test "$$conv_first_abs_path" != "/."; then \
	  conv_resolved_abs_path="$$conv_resolved_abs_path$$conv_first_abs_path"; \
    fi; \
  done; \
  conv_abs_path=$$conv_resolved_abs_path  


# conv_same_path is the same part of abs and current_path.
# if abs=/usr/bin, current_path=/usr/local, then conv_same_path=/usr
conv_get_same_path = \
  conv_same_path=""; \
  conv_tmp_abs_path=$$conv_abs_path; \
  conv_tmp_current_path=$$conv_current_path; \
  conv_rel_path=$$conv_abs_path; \
  while test -n "$$conv_tmp_abs_path"; do \
    conv_first_abs_path=`echo "$$conv_tmp_abs_path" | sed -e "$$conv_sed_get_first"`; \
	conv_tmp_abs_path=`echo "$$conv_tmp_abs_path" | sed -e "$$conv_sed_del_first"`; \
	conv_first_current_path=`echo "$$conv_tmp_current_path" | sed -e "$$conv_sed_get_first"`;\
	conv_tmp_current_path=`echo "$$conv_tmp_current_path" | sed -e "$$conv_sed_del_first"`;\
	if test "$$conv_first_abs_path" = "$$conv_first_current_path"; then \
	  conv_rel_path=`echo "$$conv_rel_path" | sed -e "$$conv_sed_del_first"`; \
	  conv_same_path="$$conv_same_path$$conv_first_abs_path"; \
	else \
	  break; \
	fi; \
  done


# delete the front '/'
conv_get_standard_relative_dir_exp = \
  conv_rel_path=`echo "$$conv_rel_path" | sed -e "$$conv_sed_del_first_slash"` 


# generate relative path expr  
conv_generate_relative_dir = \
  conv_tmp_abs_path=$$conv_abs_path; \
  conv_tmp_current_path=$$conv_current_path; \
  conv_same_path=`echo "$$conv_same_path" | sed -e "$$conv_sed_del_last_slash"`; \
  while test -n "$$conv_tmp_current_path"; do \
    if test "$$conv_tmp_current_path" = "$$conv_same_path"; then \
	  break; \
	fi; \
	conv_tmp_current_path=`echo "$$conv_tmp_current_path" | sed -e "$$conv_sed_del_last"`; \
    conv_rel_path="../$$conv_rel_path"; \
  done


# if abs is child path of current path, add './'  
conv_abs_in_current_path = \
  if test "$$conv_same_path" = "$$conv_current_path"; then \
    conv_rel_path="./$$conv_rel_path"; \
  fi  


# now this is the new am__relativize
am__relativize = \
  conv_dist=$$dir1; \
  $(convert_abs_to_rel_path); \
  dir1=$$conv_dist; \
  conv_dist=$$dir2; \
  $(convert_abs_to_rel_path); \
  dir2=$$conv_dist; \
  $(am__old__relativize)


dist-hook: gen-ChangeLog
	@set -e; echo "$(VERSION)" > $(distdir)/VERSION


gen_start_date = 2011-12-01T00:00:00
.PHONY: gen-ChangeLog
gen-ChangeLog:
	set -e;                         				\
	if test -d $(top_srcdir)/.git; then				\
	  (cd $(top_srcdir) &&                      			\
	    $(GITLOG_TO_CHANGELOG) --append-dot --tear-off		\
	    --amend=build-aux/git-log-fix  --tear-off			\
	    --since=$(gen_start_date) ) > $(distdir)/cl-t;		\
          cat $(top_srcdir)/build-aux/git-log-footer >> $(distdir)/cl-t;\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi


if HAVE_W32_SYSTEM
install-data-hook:
	set -e; \
	for i in $$($(top_srcdir)/potomo --get-linguas $(top_srcdir)/po); do \
           $(MKDIR_P) "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES" || true; \
           rm -f "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES/libgpg-error.mo" \
                                                 2>/dev/null || true; \
           $(top_srcdir)/potomo $(top_srcdir)/po/$$i.po \
              "$(DESTDIR)$(localedir)/$$i/LC_MESSAGES/libgpg-error.mo" ; \
	done
endif

stowinstall:
	$(MAKE) $(AM_MAKEFLAGS) install prefix=/usr/local/stow/libgpg-error

