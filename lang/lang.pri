# Discover restored/new TS files when qmake runs. Runtime discovery is independent.
TRANSLATIONS += $$files($$PWD/$${PROJECT_TRANSLATION_PREFIX}*.ts)
DISTFILES += $$PWD/languages.ini $$PWD/README.md

LANG_QM_FILES =
LANG_PREBUILT_QM_FILES = $$files($$PWD/*.qm)
!isEmpty(TRANSLATIONS) {
  qtPrepareTool(QMAKE_LRELEASE, lrelease)
  updateqm.input = TRANSLATIONS
  updateqm.output = $$BUILD_DIR/$$PROJECT_LANG_DIR/${QMAKE_FILE_IN_BASE}.qm
  updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
  updateqm.CONFIG += no_link target_predeps
  QMAKE_EXTRA_COMPILERS += updateqm

  for(tsfile, TRANSLATIONS) {
    qmfile = $$basename(tsfile)
    qmfile ~= s/\.ts$/.qm/
    LANG_QM_FILES += $$BUILD_DIR/$$PROJECT_LANG_DIR/$$qmfile
    LANG_PREBUILT_QM_FILES -= $$PWD/$$qmfile
  }
}

# Also accept distributed QM files without TS sources; generated ones take priority.
LANG_QM_FILES += $$LANG_PREBUILT_QM_FILES

languages.files = $$PWD/languages.ini $$LANG_QM_FILES
languages.CONFIG += no_check_exist
unix:!mac {
  languages.path = $$DATA_DIR/$$PROJECT_LANG_DIR
  INSTALLS += languages
}
win32 {
  languages.path = $$DESTDIR/$$PROJECT_LANG_DIR
  INSTALLS += languages
}
mac {
  languages.path = Contents/Resources/$$PROJECT_LANG_DIR
  QMAKE_BUNDLE_DATA += languages
}
