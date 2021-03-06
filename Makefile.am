# got some hints from https://gitorious.org/openismus-playground/examplelib/source

if HAVE_PO
SUBDIRS = po
else
SUBDIRS =
endif

SUBDIRS += lib include libwget examples src data $(LIBWGET_DOCS) tests

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information. Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libwget.pc

EXTRA_DIST = build-aux/config.rpath m4/gnulib-cache.m4

dist-hook: gen-ChangeLog

.PHONY: gen-ChangeLog check-valgrind

gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then \
	  git log --no-merges --date=short \
	    --pretty='format:%ad %an <%ae>%w(0,0,5)%+B' | sed '/^[1-9].*/G' \
	    > $(distdir)/ChangeLog; \
	fi

check-valgrind:
	TESTS_ENVIRONMENT="VALGRIND_TESTS=1" $(MAKE) check

clean-lcov:
	rm -rf wget2_base.info wget2_test.info wget2_total.info */*.gc?? lcov/
	lcov --zerocounters --directory src/ --directory libwget/

LCOV_INFO=wget2.info
check-coverage: clean clean-lcov
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage"
	lcov --capture --initial --directory src/ --directory libwget/.libs --output-file $(LCOV_INFO)
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage" VALGRIND_TESTS=0 check
	lcov --capture --directory src/ --directory libwget/.libs --output-file $(LCOV_INFO)
	lcov --remove $(LCOV_INFO) '*/test_linking.c' '*/css_tokenizer.lex' '*/<stdout>' -o $(LCOV_INFO)
	genhtml --prefix . --ignore-errors source $(LCOV_INFO) --legend --title "Wget2" --output-directory=lcov
