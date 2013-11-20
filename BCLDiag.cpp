#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>

#include "BCLDiag.hpp"

BCLDiag::BCLDiag(QWidget *parent)
  : QDialog(parent),
    m_checkingIp(false)
{
  m_keyLineEdit = new QLineEdit();

  m_statusLabel = new QLabel("Please enter your BCL Auth Key:");

  m_connectButton = new QPushButton("Connect");
  m_quitButton = new QPushButton("Quit");

  m_buttonBox = new QDialogButtonBox;
  m_buttonBox->addButton(m_connectButton, QDialogButtonBox::ActionRole);
  m_buttonBox->addButton(m_quitButton, QDialogButtonBox::RejectRole);

  connect(m_keyLineEdit, SIGNAL(textChanged(QString)), this, SLOT(enableConnectButton()));
  connect(m_connectButton, SIGNAL(clicked()), this, SLOT(connectClicked()));
  connect(m_quitButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->addWidget(m_keyLineEdit);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(m_statusLabel);
  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(m_buttonBox);
  setLayout(mainLayout);

  setWindowTitle("BCL Auth Test (v1.1)");
  setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
  setMinimumWidth(300);
  m_keyLineEdit->setFocus();

  enableConnectButton();
}

void BCLDiag::startRequest(QUrl url)
{
  QNetworkRequest request = QNetworkRequest(url);
  request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
  request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17");

  m_reply = m_qnam.get(request);
  connect(m_reply, SIGNAL(finished()), this, SLOT(httpFinished()));
}

void BCLDiag::connectClicked()
{
  m_key = m_keyLineEdit->text();
  bool validAuthKey = QRegExp("[A-Za-z0-9]{32}").exactMatch(m_key);
  if (!validAuthKey) {
    QString errorMsg;
    if (QRegExp("[A-Za-z0-9]{32}").exactMatch(m_key.trimmed())) {
      m_key = m_key.trimmed();
      validAuthKey = true;
      m_keyLineEdit->setText(m_key);
      errorMsg = QString("Your key has extra whitespace at the beggining or end.  The key can only contain 32 non-whitespace alphanumeric characters.\n\nContinuing with fixed auth key: \"%1\"").arg(m_key);
    } else if (m_key.length() != 32) {
      errorMsg = QString("Your key is %1 character%2.  The key can only contain 32 non-whitespace alphanumeric characters.").arg(QString::number(m_key.length()), m_key.length() == 1 ? "" : "s");
    } else {
      errorMsg = "The key can only contain 32 non-whitespace alphanumeric characters.";
    }
    m_statusLabel->setText("Malformed Auth Key");
    QMessageBox::warning(this, "Malformed Auth Key", errorMsg);
    if (!validAuthKey) return;
  }

  m_statusLabel->setText("Connecting...");

  if (!m_checkingIp) {
    m_statusLabel->setText("Checking IP...");
    m_checkingIp = true;
    m_url = "http://axelstudios.com/ip.php";
  } else {
    m_checkingIp = false;
    m_url = QString("http://bcl.nrel.gov/api/search/?api_version=1.1&show_rows=0&oauth_consumer_key=%1").arg(m_key);
  }

  m_connectButton->setEnabled(false);

  // schedule the request
  m_httpRequestAborted = false;
  startRequest(m_url);
}

void BCLDiag::cancelRequest()
{
  m_statusLabel->setText("Canceled.");
  m_httpRequestAborted = true;
  m_reply->abort();
  m_connectButton->setEnabled(true);
}

void BCLDiag::httpFinished()
{
  QString response = m_reply->readAll();

  if (m_httpRequestAborted) {
    m_reply->deleteLater();
    m_checkingIp = false;
    return;
  }

  if (m_checkingIp) {
    QString octet("(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])");
    bool validIp = QRegExp(QString("^%1\\.%1\\.%1\\.%1$").arg(octet)).exactMatch(response);
    m_ip = validIp ? response : "";

    m_reply->deleteLater();
    m_reply = 0;

    connectClicked();
    return;
  }

  QString statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
  QString errorString = m_reply->errorString();
  QString detailedText = QString("BCL Key: \"%1\"\n").arg(m_key);
  if (!m_ip.isEmpty()) {
    detailedText += QString("IP Address: %1\n").arg(m_ip);
  }
  if (m_reply->error() != QNetworkReply::NoError) {
    detailedText += QString("QNetworkReply Error %1: %2\n").arg(QString::number(m_reply->error()), errorString);
  }
  detailedText += QString("HTTP Status: %1\n\n").arg(!statusCode.isEmpty() ? statusCode : "N/A");
  if (m_reply->rawHeaderList().size()) {
    detailedText += "Response Headers:\n";
    for (int i=0; i<m_reply->rawHeaderList().size(); ++i) {
      detailedText += QString("%1: %2\n").arg(
          QString(m_reply->rawHeaderList()[i]),
          QString(m_reply->rawHeader(m_reply->rawHeaderList()[i])));
    }
  } else {
    detailedText += "Response Headers: N/A\n";
  }
  if (response.length()) {
    detailedText += QString("\nResponse Body:\n%1").arg(response);
  } else {
    detailedText += "\nResponse Body: N/A";
  }

  QVariant redirectionTarget = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (m_reply->error() != QNetworkReply::NoError) {
    switch(m_reply->error()) {
      case QNetworkReply::AuthenticationRequiredError:
        m_statusLabel->setText("Invalid Auth Key");
        break;
      case QNetworkReply::ConnectionRefusedError:
        m_statusLabel->setText("Network Error: Connection Refused");
        break;
      case QNetworkReply::UnknownContentError:
        m_statusLabel->setText("Network Error: Unknown Content. Verify the User-Agent header");
        break;
      case QNetworkReply::HostNotFoundError:
        if (m_ip.isEmpty()) {
          m_statusLabel->setText("Network Error: Offline.  Check your internet connection");
        } else {
          m_statusLabel->setText("Network Error: Host Not Found");
        }
        break;
      case QNetworkReply::UnknownNetworkError:
        if (errorString.startsWith("Error creating SSL context")) {
          m_statusLabel->setText("DLL Error: SSL unavailable");
        } else {
          m_statusLabel->setText("Network Error: Unknown Network Error");
        }
        break;
      default:
        if (!statusCode.isEmpty()) {
          m_statusLabel->setText(QString("Network Error: %1 - %2").arg(statusCode, errorString));
        } else {
          m_statusLabel->setText(QString("Network Error: %1").arg(errorString));
        }
    }

    QMessageBox msgBox(QMessageBox::Warning, "Connection Failed", QString("Connection failed: %1.").arg(m_statusLabel->text()), QMessageBox::NoButton, this);
    msgBox.setDetailedText(detailedText);
    msgBox.exec();

    enableConnectButton();
  } else if (!redirectionTarget.isNull()) {
    QUrl newUrl = m_url.resolved(redirectionTarget.toUrl());
    if (QMessageBox::question(this,
                              "Connection Redirect",
                              QString("Redirect to %1?").arg(newUrl.toString()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      m_url = newUrl;
      m_reply->deleteLater();
      startRequest(m_url);
      return;
    } else {
      cancelRequest();
      return;
    }
  } else {
    QString acceptableResponse("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<results><keywords> </keywords><show_rows>0</show_rows><filters></filters><result/></results>\n");
    if (response == acceptableResponse) {
      m_statusLabel->setText("Success!");
    } else {
      m_statusLabel->setText("Unrecognized Response.");
      QMessageBox msgBox(QMessageBox::Warning,
                         "Unrecognized Response",
                         "Server returned OK, but the response body is unrecognized",
                         QMessageBox::NoButton,
                         this);
      msgBox.setDetailedText(detailedText);
      msgBox.exec();
    }
    enableConnectButton();
  }

  m_reply->deleteLater();
  m_reply = 0;
}

void BCLDiag::enableConnectButton()
{
  m_connectButton->setEnabled(!m_keyLineEdit->text().isEmpty());
}
