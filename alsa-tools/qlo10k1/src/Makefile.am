#SUBDIRS = dlgs icons

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = qlo10k1

AM_CXXFLAGS= @QT_CXXFLAGS@ @LD10K1_CFLAGS@

# the libraries to link against.
qlo10k1_LDADD = $(LDADD) @QT_LDADD@ @LD10K1_LIBS@

EXTRA_DIST = dlgs/connectionwizardbase.ui dlgs/filecontroldialogbase.ui dlgs/filecontrolsdialogbase.ui \
	dlgs/loadpatchdialogbase.ui dlgs/mainwndbase.ui dlgs/patchdialogbase.ui dlgs/preferencesbase.ui \
	dlgs/newiodialogbase.ui dlgs/aboutdialogbase.ui dlgs/transformpatchbase.ui\
	icons/routing_fx.xpm \
	icons/routing_in.xpm \
	icons/routing_out.xpm \
	icons/routing_route.xpm \
	icons/routing_effect.xpm \
	icons/routing_effect_stack.xpm \
	icons/routing_zoom_plus.xpm \
	icons/routing_zoom_minus.xpm \
	icons/routing_normal.xpm

# these are the headers for your project
noinst_HEADERS = 

%.moc.cpp: %.h
	$(MOC) -o $@ $<

%.h: %.ui
	$(UIC) -o $@ $<

%.cpp: %.ui
	$(UIC) -o $@ -impl $*.h $<

qlo10k1_UI = dlgs/connectionwizardbase.h \
	dlgs/connectionwizardbase.cpp \
	\
	dlgs/filecontroldialogbase.h \
	dlgs/filecontroldialogbase.cpp \
	\
	dlgs/filecontrolsdialogbase.h \
	dlgs/filecontrolsdialogbase.cpp \
	\
	dlgs/loadpatchdialogbase.h \
	dlgs/loadpatchdialogbase.cpp \
	\
	dlgs/transformpatchbase.h \
	dlgs/transformpatchbase.cpp \
	\
	dlgs/mainwndbase.h \
	dlgs/mainwndbase.cpp \
	\
	dlgs/patchdialogbase.h \
	dlgs/patchdialogbase.cpp \
	\
	dlgs/preferencesbase.h \
	dlgs/preferencesbase.cpp \
	\
	dlgs/newiodialogbase.h \
	dlgs/newiodialogbase.cpp \
	\
	dlgs/aboutdialogbase.h \
	dlgs/aboutdialogbase.cpp

qlo10k1_MOC = dlgs/connectionwizardbase.moc.cpp \
	dlgs/filecontroldialogbase.moc.cpp \
	dlgs/filecontrolsdialogbase.moc.cpp \
	dlgs/loadpatchdialogbase.moc.cpp \
	dlgs/mainwndbase.moc.cpp \
	dlgs/patchdialogbase.moc.cpp \
	dlgs/preferencesbase.moc.cpp \
	dlgs/newiodialogbase.moc.cpp \
	dlgs/aboutdialogbase.moc.cpp \
	dlgs/transformpatchbase.moc.cpp \
	\
	connectionwizard.moc.cpp \
	filecontroldialog.moc.cpp \
	loadpatchdialog.moc.cpp \
	mainwnd.moc.cpp \
	patchdialog.moc.cpp \
	preferences.moc.cpp \
	routingwidget.moc.cpp \
	newiodialog.moc.cpp \
	aboutdialog.moc.cpp \
	transformpatchdialog.moc.cpp

BUILT_SOURCES = $(qlo10k1_MOC) $(qlo10k1_UI)
CLEANFILES = $(BUILT_SOURCES)

# which sources should be compiled for qlo10k1
# emu10k1file.cpp emu10k1file.h
qlo10k1_SOURCES = app_global.h connectionwizard.cpp connectionwizard.h \
	filecontroldialog.cpp filecontroldialog.h ld10k1file.cpp ld10k1file.h \
	loadpatchdialog.cpp loadpatchdialog.h main.cpp \
	mainwnd.cpp mainwnd.h patchdialog.cpp patchdialog.h preferences.cpp preferences.h \
	routingwidget.h routingwidget.cpp structure.cpp structure.h structure_patch.cpp structure_patch.h \
	structure_io.cpp structure_io.h structure_link.cpp structure_link.h\
	structure_aa.cpp strglobal.h strglobal.cpp routingwidget_new.cpp routingwidget_menu.cpp \
	newiodialog.h newiodialog.cpp strparam.h strparam.cpp \
	aboutdialog.h aboutdialog.cpp transformpatchdialog.h transformpatchdialog.cpp \
	custom_colors.h
#	\
#	$(qlo10k1_UI) \
#       $(qlo10k1_MOC)

nodist_qlo10k1_SOURCES = $(qlo10k1_UI) \
        $(qlo10k1_MOC)

dlgs/connectionwizardbase.h : dlgs/connectionwizardbase.ui
dlgs/connectionwizardbase.cpp : dlgs/connectionwizardbase.h dlgs/connectionwizardbase.ui

dlgs/filecontroldialogbase.h : dlgs/filecontroldialogbase.ui
dlgs/filecontroldialogbase.cpp : dlgs/filecontroldialogbase.h dlgs/filecontroldialogbase.ui

dlgs/filecontrolsdialogbase.h : dlgs/filecontrolsdialogbase.ui
dlgs/filecontrolsdialogbase.cpp : dlgs/filecontrolsdialogbase.h dlgs/filecontrolsdialogbase.ui

dlgs/loadpatchdialogbase.h : dlgs/loadpatchdialogbase.ui
dlgs/loadpatchdialogbase.cpp : dlgs/loadpatchdialogbase.h dlgs/loadpatchdialogbase.ui

dlgs/transformpatchbase.h : dlgs/transformpatchbase.ui
dlgs/transformpatchbase.cpp : dlgs/transformpatchbase.h dlgs/transformpatchbase.ui

dlgs/mainwndbase.h : dlgs/mainwndbase.ui
dlgs/mainwndbase.cpp : dlgs/mainwndbase.h dlgs/mainwndbase.ui

dlgs/patchdialogbase.h : dlgs/patchdialogbase.ui
dlgs/patchdialogbase.cpp : dlgs/patchdialogbase.h dlgs/patchdialogbase.ui

dlgs/preferencesbase.h : dlgs/preferencesbase.ui
dlgs/preferencesbase.cpp : dlgs/preferencesbase.h dlgs/preferencesbase.ui

dlgs/newiodialogbase.h : dlgs/newiodialogbase.ui
dlgs/newiodialogbase.cpp : dlgs/newiodialogbase.h dlgs/newiodialogbase.ui

dlgs/aboutdialogbase.h : dlgs/aboutdialogbase.ui
dlgs/aboutdialogbase.cpp : dlgs/aboutdialogbase.h dlgs/aboutdialogbase.ui