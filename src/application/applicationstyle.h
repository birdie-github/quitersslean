#ifndef APPLICATIONSTYLE_H
#define APPLICATIONSTYLE_H

#include <QList>
#include <QString>

struct ApplicationStyle {
  QString id;
  QString name;
  QString fileName;
  QString sheet;
  bool darkColors = false;
  bool isDefault = false;
};

// Reads the runtime directory each time; no cached file list or widget ownership.
namespace ApplicationStyles {
QList<ApplicationStyle> discover(const QString &directory);
ApplicationStyle systemDefault();
}

#endif
