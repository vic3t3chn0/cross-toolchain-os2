include $(call select_from_repositories,lib/import/import-qt5_quick.mk)

SHARED_LIB = yes

include $(REP_DIR)/lib/mk/qt5_quick_generated.inc

# remove unneeded files to prevent moc warnings
COMPILER_MOC_HEADER_MAKE_ALL_FILES_FILTER_OUT = \
  moc_qquickaccessibleattached_p.cpp \
  moc_qquickprofiler_p.cpp

QT_INCPATH += qtdeclarative/src/quick/items

include $(REP_DIR)/lib/mk/qt5.inc

LIBS += qt5_core qt5_gui qt5_network qt5_qml

CC_CXX_WARN_STRICT =
