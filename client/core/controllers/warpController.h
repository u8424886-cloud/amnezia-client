#ifndef WARPCONTROLLER_H
#define WARPCONTROLLER_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Генератор конфигов Cloudflare WARP для AmneziaWG.
//
// Ключи генерируются локально на машине пользователя: приватный ключ
// остаётся в конфиге, наружу уходит только публичный — для регистрации
// устройства в бесплатном WARP через api.cloudflareclient.com.
//
// Эндпоинты Cloudflare и набор портов рандомизируются при каждом запуске:
// у разных устройств получается разная сигнатура, которую сложнее
// обнаружить фильтру по протоколу.
//
// Запросы выполняются асинхронно, интерфейс не блокируется.

namespace amnezia;

class WarpController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantList configs READ configs NOTIFY configsChanged)
    Q_PROPERTY(QString configsPath READ configsPath CONSTANT)
    Q_PROPERTY(bool hasConfigs READ hasConfigs NOTIFY configsChanged)

public:
    explicit WarpController(QObject *parent = nullptr);
    ~WarpController() = default;

    // Создаёт count конфигов, предыдущие удаляет.
    Q_INVOKABLE void generate(int count = 5);

    // Текст конфига по индексу — для передачи в ImportController.
    Q_INVOKABLE QString configText(int index) const;
    Q_INVOKABLE QString configName(int index) const;

    // Кладёт текст выбранного конфига в буфер обмена.
    Q_INVOKABLE void copyToClipboard(int index);

    // Открыть папку с конфигами в файловом менеджере.
    Q_INVOKABLE void openConfigsFolder();

    bool busy() const { return m_busy; }
    QString statusText() const { return m_statusText; }
    QVariantList configs() const { return m_exposed; }
    QString configsPath() const;
    bool hasConfigs() const { return !m_configs.isEmpty(); }

signals:
    void busyChanged();
    void statusTextChanged();
    void configsChanged();

private:
    struct GeneratedConfig
    {
        QString name;
        QString endpoint;
        QString text;
        QString filePath;
    };

    struct WarpKeys
    {
        QString privateKey;
        QString peerPublicKey;
        QString ipv4;
        QString ipv6;
    };

    void setBusy(bool value);
    void setStatusText(const QString &text);

    // Регистрирует устройство: сначала priv/pub -> Cloudflare, затем PATCH.
    void requestOne(int index);
    void checkFinished();
    void clearOldConfigs();
    void writeConfigs(const QList<GeneratedConfig> &configs);

    static QNetworkRequest apiRequest(const QString &path);
    static QNetworkRequest mirrorRequest(const QString &url);
    static QVariantMap parseReply(QNetworkReply *reply);

    static QPair<QByteArray, QByteArray> generateKeyPair();
    static QByteArray x25519(const QByteArray &scalar, const QByteArray &uPoint);
    static QString randomEndpoint();
    static QString randomHex(int bytes);
    static QString extractEndpoint(const QString &conf);

    QString buildConf(const WarpKeys &keys, bool sipMask = false);

    QNetworkAccessManager m_manager;
    QHash<int, QString> m_pendingKeys;

    int m_targetCount = 5;
    int m_pending = 0;
    int m_failures = 0;

    bool m_busy = false;
    QString m_statusText;

    QList<GeneratedConfig> m_generated;  // свежая партия, ещё без путей
    QList<GeneratedConfig> m_configs;     // готово к выдаче в QML
    QVariantList m_exposed;               // то же, но в виде QVariantList
};

#endif // WARPCONTROLLER_H
