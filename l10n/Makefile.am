regionsdir = $(kde_locale)/l10n
regions_DATA = northafrica.desktop westafrica.desktop \
	southafrica.desktop centralafrica.desktop \
	caribbean.desktop oceania.desktop \
	centralamerica.desktop northamerica.desktop southamerica.desktop \
	easteurope.desktop westeurope.desktop \
	centraleurope.desktop northeurope.desktop southeurope.desktop \
	southasia.desktop southeastasia.desktop eastasia.desktop \
	centralasia.desktop middleeast.desktop

EXTRA_DIST = README

install-data-local:
	@list=`(cd $(srcdir) && ls -1d * | grep -v CVS | grep -v '\.svn')` ;\
	for i in $$list; do if test -d $(srcdir)/$$i; then \
	  echo $(mkinstalldirs) $(DESTDIR)$(kde_locale)/l10n/$$i ;\
	  $(mkinstalldirs) $(DESTDIR)$(kde_locale)/l10n/$$i; \
	  echo $(INSTALL_DATA) $(srcdir)/$$i/entry.desktop $(DESTDIR)$(kde_locale)/l10n/$$i ;\
	  $(INSTALL_DATA) $(srcdir)/$$i/entry.desktop $(DESTDIR)$(kde_locale)/l10n/$$i ;\
	  echo $(INSTALL_DATA) $(srcdir)/$$i/flag.png $(DESTDIR)$(kde_locale)/l10n/$$i ;\
	  $(INSTALL_DATA) $(srcdir)/$$i/flag.png $(DESTDIR)$(kde_locale)/l10n/$$i ;\
	fi; done
