#ifndef BCLDIAG_HPP
#define BCLDIAG_HPP

#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrl>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QNetworkReply;

class BCLDiag : public QDialog
{
  Q_OBJECT

public:
  BCLDiag(QWidget *parent = 0);

  ~BCLDiag() {};

  void startRequest(const QUrl url);

private slots:
  void connectClicked();
  void cancelRequest();
  void httpFinished();
  void enableConnectButton();

private:
  QLabel *m_statusLabel;
  QLineEdit *m_keyLineEdit;
  QDialogButtonBox *m_buttonBox;
  QPushButton *m_connectButton;
  QPushButton *m_quitButton;

  QUrl m_url;
  QUrl m_originalUrl;
  QNetworkAccessManager m_qnam;
  QNetworkReply *m_reply;

  bool m_httpRequestAborted;
  bool m_checkingIp;

  QString m_key;
  QString m_ip;
};

#endif // BCLDIAG_HPP
