/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2020 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
* ============================================================ */
#include "networkmanager.h"
#include "networkpolicy.h"

#include "mainapplication.h"
#include "common.h"
#include "settings.h"
#include "authenticationdialog.h"
#include "sslerrordialog.h"

#include <QNetworkReply>
#include <QSslConfiguration>
#include <QDebug>

static QString fileNameForCert(const QSslCertificate &cert)
{
  QString certFileName = SslErrorDialog::certificateItemText(cert);
  certFileName.remove(QLatin1Char(' '));
  certFileName.append(QLatin1String(".crt"));
  certFileName = Common::filterCharsFromFilename(certFileName);

  while (certFileName.startsWith(QLatin1Char('.'))) {
    certFileName = certFileName.mid(1);
  }

  return certFileName;
}

NetworkManager::NetworkManager(bool isThread, QObject* parent)
  : QNetworkAccessManager(parent)
  , ignoreAllWarnings_(false)
{
  setCookieJar(mainApp->cookieJar());
  // CookieJar is shared between NetworkManagers
  mainApp->cookieJar()->setParent(0);

#ifndef QT_NO_NETWORKPROXY
  qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
  qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
#endif

  connect(this, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
          this, SLOT(slotSslError(QNetworkReply*, QList<QSslError>)));

  if (isThread) {
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            mainApp->networkManager(), SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)),
            Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            mainApp->networkManager(), SLOT(slotProxyAuthentication(QNetworkProxy,QAuthenticator*)),
            Qt::BlockingQueuedConnection);

  } else {
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
            SLOT(slotAuthentication(QNetworkReply*,QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(slotProxyAuthentication(QNetworkProxy,QAuthenticator*)));

    loadSettings();
  }
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::loadSettings()
{
  // Load roots from Qt's platform trust store, not the retired bundled file.
  loadCertificates();
}

void NetworkManager::loadCertificates()
{
  Settings settings;
  settings.beginGroup("SSL-Configuration");
  certPaths_ = settings.value("CACertPaths", QStringList()).toStringList();
  ignoreAllWarnings_ = settings.value("IgnoreAllSSLWarnings", false).toBool();
  settings.endGroup();

  localCerts_.clear();

  // CA Certificates
  caCerts_ = QSslConfiguration::systemCaCertificates();

  foreach (const QString &path, certPaths_) {
    QDirIterator it(path, QDir::Files, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
      QString filePath = it.next();
      if (!filePath.endsWith(QLatin1String(".crt"))) {
        continue;
      }

      QFile file(filePath);
      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        caCerts_ += QSslCertificate::fromData(file.readAll(), QSsl::Pem);
      }
    }
  }
  // Local Certificates
  QDirIterator it_(mainApp->dataDir() + "/certificates", QDir::Files, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
  while (it_.hasNext()) {
    QString filePath = it_.next();
    // Do not re-import the obsolete bundled roots from an existing profile.
    if (!filePath.endsWith(QLatin1String(".crt")) ||
        QFileInfo(filePath).fileName() == QLatin1String("ca-bundle.crt")) {
      continue;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      localCerts_ += QSslCertificate::fromData(file.readAll(), QSsl::Pem);
    }
  }

  QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
  ssl.setCaCertificates(caCerts_ + localCerts_);
  QSslConfiguration::setDefaultConfiguration(ssl);

}

/** @brief Request authentification
 *---------------------------------------------------------------------------*/
void NetworkManager::slotAuthentication(QNetworkReply *reply, QAuthenticator *auth)
{
  AuthenticationDialog *authenticationDialog =
      new AuthenticationDialog(reply->url(), auth);

  if (!authenticationDialog->save_->isChecked())
    authenticationDialog->exec();

  delete authenticationDialog;
}
/** @brief Request proxy authentification
 *---------------------------------------------------------------------------*/
void NetworkManager::slotProxyAuthentication(const QNetworkProxy &proxy, QAuthenticator *auth)
{
  AuthenticationDialog *authenticationDialog =
      new AuthenticationDialog(proxy.hostName(), auth);

  if (!authenticationDialog->save_->isChecked())
    authenticationDialog->exec();

  delete authenticationDialog;
}

static inline uint qHash(const QSslCertificate &cert)
{
  return qHash(cert.toPem());
}

void NetworkManager::slotSslError(QNetworkReply *reply, QList<QSslError> errors)
{
  if (ignoreAllWarnings_ || reply->property("downloadReply").toBool() ||
      (mainApp->networkManager() != this)) {
    reply->ignoreSslErrors(errors);
    return;
  }

  QHash<QSslCertificate, QStringList> errorHash;
  foreach (const QSslError &error, errors) {
    // Weird behavior on Windows
    if (error.error() == QSslError::NoError) {
      continue;
    }

    QSslCertificate cert = error.certificate();

    if (errorHash.contains(cert)) {
      errorHash[cert].append(error.errorString());
    }
    else {
      errorHash.insert(cert, QStringList(error.errorString()));
    }
  }

  // User already rejected those certs
  if (containsRejectedCerts(errorHash.keys())) {
    return;
  }

  QString title = tr("SSL Certificate Error!");
  QString text1 = QString(tr("The \"%1\" server has the following errors in the SSL certificate:")).
      arg(reply->url().host());

  QString certs;

  QHash<QSslCertificate, QStringList>::const_iterator i = errorHash.constBegin();
  while (i != errorHash.constEnd()) {
    const QSslCertificate cert = i.key();
    const QStringList errors = i.value();

    if (localCerts_.contains(cert) || tempAllowedCerts_.contains(cert) || errors.isEmpty()) {
      ++i;
      continue;
    }

    certs += "<ul><li>";
    certs += tr("<b>Organization: </b>") +
        SslErrorDialog::clearCertSpecialSymbols(cert.subjectInfo(QSslCertificate::Organization));
    certs += "</li><li>";
    certs += tr("<b>Domain Name: </b>") +
        SslErrorDialog::clearCertSpecialSymbols(cert.subjectInfo(QSslCertificate::CommonName));
    certs += "</li><li>";
    certs += tr("<b>Expiration Date: </b>") +
        cert.expiryDate().toString("hh:mm:ss dddd d. MMMM yyyy");
    certs += "</li></ul>";

    certs += "<ul>";
    foreach (const QString &error, errors) {
      certs += "<li>";
      certs += tr("<b>Error: </b>") + error;
      certs += "</li>";
    }
    certs += "</ul>";

    ++i;
  }

  QString text2 = tr("Would you like to make an exception for this certificate?");
  QString message = QString("<b>%1</b><p>%2</p>%3<p>%4</p><br>").arg(title, text1, certs, text2);

  if (!certs.isEmpty())  {
    SslErrorDialog dialog(mainApp->mainWindow());
    dialog.setText(message);
    dialog.exec();

    switch (dialog.result()) {
    case SslErrorDialog::Yes:
      foreach (const QSslCertificate &cert, errorHash.keys()) {
        if (!localCerts_.contains(cert)) {
          addLocalCertificate(cert);
        }
      }
      break;
    case SslErrorDialog::OnlyForThisSession:
      foreach (const QSslCertificate &cert, errorHash.keys()) {
        if (!tempAllowedCerts_.contains(cert)) {
          tempAllowedCerts_.append(cert);
        }
      }
      break;
    default:
      // To prevent asking user more than once for the same certificate
      addRejectedCerts(errorHash.keys());
      return;
    }
  }

  reply->ignoreSslErrors(errors);
}

QNetworkReply *NetworkManager::createRequest(QNetworkAccessManager::Operation op,
                                             const QNetworkRequest &request,
                                             QIODevice *outgoingData)
{
  if (!NetworkPolicy::isRequestUrl(request.url()))
    return new NetworkPolicy::RejectedReply(op, request, this);
  QNetworkRequest checked(request);
  // Feed, favicon and download callers resolve and validate redirects themselves.
  checked.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
  return QNetworkAccessManager::createRequest(op, checked, outgoingData);
}

void NetworkManager::addRejectedCerts(const QList<QSslCertificate> &certs)
{
  foreach (const QSslCertificate &cert, certs) {
    if (!rejectedSslCerts_.contains(cert)) {
      rejectedSslCerts_.append(cert);
    }
  }
}

bool NetworkManager::containsRejectedCerts(const QList<QSslCertificate> &certs)
{
  int matches = 0;

  foreach (const QSslCertificate &cert, certs) {
    if (rejectedSslCerts_.contains(cert)) {
      ++matches;
    }
  }

  return matches == certs.count();
}

void NetworkManager::addLocalCertificate(const QSslCertificate &cert)
{
  localCerts_.append(cert);
  QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
  ssl.addCaCertificate(cert);
  QSslConfiguration::setDefaultConfiguration(ssl);

  QDir dir(mainApp->dataDir());
  if (!dir.exists("certificates")) {
    dir.mkdir("certificates");
  }

  QString certFileName = fileNameForCert(cert);
  QString fileName = Common::ensureUniqueFilename(mainApp->dataDir() + "/certificates/" + certFileName);

  QFile file(fileName);
  if (file.open(QFile::WriteOnly)) {
    file.write(cert.toPem());
    file.close();
  }
  else {
    qWarning() << "NetworkManager::addLocalCertificate cannot write to file: " << fileName;
  }
}

void NetworkManager::removeLocalCertificate(const QSslCertificate &cert)
{
  localCerts_.removeOne(cert);

  QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
  ssl.setCaCertificates(caCerts_ + localCerts_);
  QSslConfiguration::setDefaultConfiguration(ssl);

  // Delete cert file from profile
  bool deleted = false;
  QDirIterator it(mainApp->dataDir() + "/certificates", QDir::Files, QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString filePath = it.next();
    const QList<QSslCertificate> &certs = QSslCertificate::fromPath(filePath);
    if (certs.isEmpty()) {
      continue;
    }

    const QSslCertificate cert_ = certs.at(0);
    if (cert == cert_) {
      QFile file(filePath);
      if (!file.remove()) {
        qWarning() << "NetworkManager::removeLocalCertificate cannot remove file" << filePath;
      }

      deleted = true;
      break;
    }
  }

  if (!deleted) {
    qWarning() << "NetworkManager::removeLocalCertificate cannot remove certificate";
  }
}
