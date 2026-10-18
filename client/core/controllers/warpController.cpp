#include "warpController.h"

#include <QDebug>
#include <utility>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace
{

constexpr char kApiBase[] = "https://api.cloudflareclient.com/v0i1909051800";
constexpr char kUserAgent[] = "CloudflareWarp/1.0";
constexpr int kTimeoutMs = 20000;

// (comment translated to English)
const QStringList kEndpointPrefixes = {
    "162.159.192.", "162.159.195.", "engage.cloudflareclient.com",
    "8.6.112.",    "8.34.70.",   "8.34.146.",
    "8.35.211.",   "8.39.125.",  "8.39.204.",
    "8.39.214.",   "8.47.69.",   "188.114.96.",
    "188.114.97.", "188.114.98.", "188.114.99."};

// (comment translated to English)
// (comment translated to English)
const QVector<int> kEndpointPorts = {
    500,  854,  859,  864,  878,  880,  890,  891,  894,  903,  908,
    928,  934,  939,  942,  943,  945,  946,  955,  968,  987,  988,
    1002, 1010, 1014, 1018, 1070, 1074, 1180, 1387, 1701, 1843, 2371,
    2408, 2506, 3138, 3476, 3581, 3854, 4177, 4198, 4233, 4500, 5279,
    5956, 7103, 7152, 7156, 7281, 7559, 8319, 8742, 8854, 8886};

// (comment translated to English)
// (comment translated to English)
const char kSipInvite[] =
    "494e56495445207369703a626f624062696c6f78692e636f6d205349502f322e300d0a"
    "5669613a205349502f322e302f55445020706333332e61746c616e74612e636f6d3b6272616e"
    "63683d7a39684734624b3737366173646864730d0a4d61782d466f7277617264733a2037300d"
    "0a546f3a20426f62203c7369703a626f624062696c6f78692e636f6d3e0d0a46726f6d3a2041"
    "6c696365203c7369703a616c6963654061746c616e74612e636f6d3e3b7461673d3139323833"
    "30313737340d0a43616c6c2d49443a20613834623463373665363637313040706333332e6174"
    "6c616e74612e636f6d0d0a435365713a2033313431353920494e564954450d0a436f6e746163"
    "743a203c7369703a616c69636540706333332e61746c616e74612e636f6d3e0d0a436f6e74656e"
    "742d547970653a206170706c69636174696f6e2f7364700d0a436f6e74656e742d4c656e677468"
    "3a20300d0a0d0a";

// (comment translated to English)
const char kSipTrying[] =
    "5349502f322e302031303020547279696e670d0a5669613a205349502f322e302f5544502070"
    "6333332e61746c616e74612e636f6d3b6272616e63683d7a39684734624b3737366173646864"
    "730d0a546f3a20426f62203c7369703a626f624062696c6f78692e636f6d3e0d0a46726f6d3a20"
    "416c696365203c7369703a616c6963654061746c616e74612e636f6d3e3b7461673d3139323833"
    "30313737340d0a43616c6c2d49443a20613834623463373665363637313040706333332e6174"
    "6c616e74612e636f6d0d0a435365713a2033313431353920494e564954450d0a436f6e74616374"
    "3a203c7369703a616c69636540706333332e61746c616e74612e636f6d3e0d0a436f6e74656e74"
    "2d4c656e6774683a20300d0a0d0a";

int randomInt(int min, int max)
{
    return QRandomGenerator::global()->bounded(min, max + 1);
}

// (comment translated to English)
QString withPrefix(const QString &value, const QString &fallback)
{
    const QString v = value.isEmpty() ? fallback : value;
    if (v.contains(QLatin1Char('/'))) {
        return v;
    }
    return v + (v.contains(QLatin1Char(':')) ? QStringLiteral("/128") : QStringLiteral("/32"));
}

}  // namespace

WarpController::WarpController(QObject *parent) : QObject(parent) {}

// --------------------------------------------------------------------------
// (comment translated to English)
// --------------------------------------------------------------------------

QByteArray WarpController::x25519(const QByteArray &scalar, const QByteArray &uPoint)
{
    if (scalar.size() != 32 || uPoint.size() != 32) {
        return {};
    }

    EVP_PKEY *priv = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_X25519, nullptr,
        reinterpret_cast<const unsigned char *>(scalar.constData()), scalar.size());
    if (!priv) {
        return {};
    }

    EVP_PKEY *pub = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_X25519, nullptr,
        reinterpret_cast<const unsigned char *>(uPoint.constData()), uPoint.size());
    if (!pub) {
        EVP_PKEY_free(priv);
        return {};
    }

    QByteArray out;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(priv, nullptr);
    if (ctx && EVP_PKEY_derive_init(ctx) == 1 && EVP_PKEY_derive_set_peer(ctx, pub) == 1) {
        QByteArray buf(32, Qt::Uninitialized);
        size_t len = buf.size();
        if (EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char *>(buf.data()), &len) == 1) {
            out = buf;
        }
    }
    if (ctx) {
        EVP_PKEY_CTX_free(ctx);
    }
    EVP_PKEY_free(pub);
    EVP_PKEY_free(priv);

    return out;
}

QPair<QByteArray, QByteArray> WarpController::generateKeyPair()
{
    QByteArray priv(32, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(priv.data()), priv.size()) != 1) {
        return {};
    }

    // (comment translated to English)
    QByteArray basePoint(32, Qt::Uninitialized);
    basePoint[0] = 0x09;

    const QByteArray pub = x25519(priv, basePoint);
    if (pub.isEmpty()) {
        return {};
    }
    return { priv.toBase64(), pub.toBase64() };
}

// --------------------------------------------------------------------------
// HTTP
// --------------------------------------------------------------------------

QNetworkRequest WarpController::apiRequest(const QString &path)
{
    QNetworkRequest request{QUrl(QString::fromUtf8(kApiBase) + path)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromUtf8(kUserAgent));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(kTimeoutMs);
    return request;
}

QNetworkRequest WarpController::mirrorRequest(const QString &url)
{
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromUtf8(kUserAgent));
    request.setRawHeader("X-Client", "WARP");
    request.setTransferTimeout(kTimeoutMs);
    return request;
}

QVariantMap WarpController::parseReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "WARP request failed:" << reply->errorString();
        return {};
    }
    const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    if (obj.contains(QStringLiteral("error"))) {
        qWarning() << "WARP API error:" << obj.value("error").toString();
        return {};
    }
    return obj.toVariantMap();
}

// --------------------------------------------------------------------------
// (comment translated to English)
// --------------------------------------------------------------------------

void WarpController::generate(int count)
{
    if (m_busy) {
        return;
    }

    m_targetCount = qBound(1, count, 10);
    m_pending = 0;
    m_failures = 0;
    m_generated.clear();
    m_pendingKeys.clear();

    clearOldConfigs();
    setBusy(true);
    setStatusText(tr("Generating configs..."));
    emit configsChanged();

    for (int i = 0; i < m_targetCount; ++i) {
        requestOne(i);
    }
}

void WarpController::requestOne(int index)
{
    const auto keyPair = generateKeyPair();
    if (keyPair.first.isEmpty()) {
        ++m_failures;
        checkFinished();
        return;
    }

    m_pendingKeys.insert(index, QString::fromLatin1(keyPair.first));
    ++m_pending;

    QVariantMap body;
    body["install_id"] = QString();
    body["tos"] = "2024-01-01T00:00:00.000Z";
    body["key"] = QString::fromLatin1(keyPair.second);
    body["fcm_token"] = QString();
    body["type"] = "ios";
    body["locale"] = "en_US";

    QNetworkReply *reply = m_manager.post(apiRequest(QStringLiteral("reg")),
                                           QJsonDocument(body.toJsonObject()).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        const QVariantMap reg = parseReply(reply);
        reply->deleteLater();

        const QVariantMap result = reg.value(QStringLiteral("result")).toMap();
        const QString deviceId = result.value(QStringLiteral("id")).toString();
        const QString token = result.value(QStringLiteral("token")).toString();

        if (deviceId.isEmpty() || token.isEmpty()) {
            qWarning() << "WARP reg failed, index" << index;
            ++m_failures;
            checkFinished();
            return;
        }

        QVariantMap enableBody;
        enableBody["warp_enabled"] = true;

        QNetworkRequest enableRequest = apiRequest(QStringLiteral("reg/") + deviceId);
        enableRequest.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());

        QNetworkReply *enableReply = m_manager.patch(
            enableRequest, QJsonDocument(enableBody.toJsonObject()).toJson());

        connect(enableReply, &QNetworkReply::finished, this, [this, enableReply, index]() {
            const QVariantMap enabled = parseReply(enableReply);
            enableReply->deleteLater();

            const QVariantMap config =
                enabled.value(QStringLiteral("result")).toMap().value(QStringLiteral("config")).toMap();
            const QVariantList peers = config.value(QStringLiteral("peers")).toList();

            if (peers.isEmpty()) {
                qWarning() << "WARP peers empty, index" << index;
                ++m_failures;
                checkFinished();
                return;
            }

            const QVariantMap peer = peers.first().toMap();
            const QVariantMap addresses =
                config.value(QStringLiteral("interface")).toMap().value(QStringLiteral("addresses")).toMap();

            WarpKeys keys;
            keys.privateKey = m_pendingKeys.take(index);
            keys.peerPublicKey = peer.value(QStringLiteral("public_key")).toString();
            keys.ipv4 = withPrefix(addresses.value(QStringLiteral("v4")).toString(), QStringLiteral("172.16.0.2"));
            keys.ipv6 = withPrefix(addresses.value(QStringLiteral("v6")).toString(), QStringLiteral("2606:4700:110::1"));

            if (keys.privateKey.isEmpty() || keys.peerPublicKey.isEmpty()) {
                ++m_failures;
                checkFinished();
                return;
            }

            GeneratedConfig cfg;
            cfg.name = tr("WARP %1").arg(index + 1);
            // (comment translated to English)
            cfg.text = buildConf(keys, index % 2 == 1);
            cfg.endpoint = extractEndpoint(cfg.text);
            m_generated.append(cfg);

            checkFinished();
        });
    });
}

void WarpController::checkFinished()
{
    if (--m_pending > 0) {
        return;
    }

    m_pending = 0;

    if (m_generated.isEmpty()) {
        setStatusText(tr("Failed to get WARP keys. Try again later."));
        setBusy(false);
        return;
    }

    std::sort(m_generated.begin(), m_generated.end(),
              [](const GeneratedConfig &a, const GeneratedConfig &b) { return a.name < b.name; });

    writeConfigs(m_generated);

    m_exposed.clear();
    for (const GeneratedConfig &cfg : std::as_const(m_configs)) {
        QVariantMap map;
        map[QStringLiteral("name")] = cfg.name;
        map[QStringLiteral("endpoint")] = cfg.endpoint;
        map[QStringLiteral("fileName")] = QFileInfo(cfg.filePath).fileName();
        m_exposed.append(map);
    }

    QString status = tr("Done: %1 configs").arg(m_configs.size());
    if (m_failures > 0) {
        status += tr(" (errors: %1)").arg(m_failures);
    }
    setStatusText(status);
    emit configsChanged();
    setBusy(false);
}

// --------------------------------------------------------------------------
// (comment translated to English)
// --------------------------------------------------------------------------

QString WarpController::randomEndpoint()
{
    const QString prefix =
        kEndpointPrefixes.at(QRandomGenerator::global()->bounded(kEndpointPrefixes.size()));
    const QString host = (prefix == QLatin1String("engage.cloudflareclient.com"))
                             ? prefix
                             : prefix + QString::number(randomInt(10, 99));
    const int port = kEndpointPorts.at(QRandomGenerator::global()->bounded(kEndpointPorts.size()));
    return QStringLiteral("%1:%2").arg(host, QString::number(port));
}

QString WarpController::extractEndpoint(const QString &conf)
{
    static const QRegularExpression re(QStringLiteral("^Endpoint = (\\S+)"),
                                       QRegularExpression::MultilineOption);
    const auto match = re.match(conf);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString WarpController::randomHex(int bytes)
{
    QByteArray raw(bytes, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(raw.data()), raw.size()) != 1) {
        return {};
    }
    return QString::fromLatin1(raw.toHex());
}

QString WarpController::buildConf(const WarpKeys &keys, bool sipMask)
{
    QString conf;
    conf += QStringLiteral("[Interface]\n");
    conf += QStringLiteral("PrivateKey = %1\n").arg(keys.privateKey);
    conf += QStringLiteral("Address = %1, %2\n").arg(keys.ipv4, keys.ipv6);
    conf += QStringLiteral("DNS = 1.1.1.1\n");
    conf += QStringLiteral("MTU = 1280\n");
    conf += QStringLiteral("S1 = 0\nS2 = 0\nS3 = 0\nS4 = 0\n");
    conf += QStringLiteral("Jc = 4\nJmin = 40\nJmax = 70\n");
    conf += QStringLiteral("H1 = 1\nH2 = 2\nH3 = 3\nH4 = 4\n");

    if (sipMask) {
        conf += QStringLiteral("I1 = <b 0x%1>\n").arg(QLatin1String(kSipInvite));
        conf += QStringLiteral("I2 = <b 0x%1>\n").arg(QLatin1String(kSipTrying));
    } else {
        // (comment translated to English)
        conf += QStringLiteral("I1 = <b 0x%1>\n").arg(randomHex(1250));
    }

    // (comment translated to English)
    conf += QStringLiteral("ContentPaddingAddition = %1\n").arg(randomInt(27, 97));
    conf += QStringLiteral("MaxHandshakeAttempts = %1\n").arg(randomInt(18, 26));
    conf += QStringLiteral("KeepaliveTimeout = %1\n").arg(randomInt(9, 16));
    conf += QStringLiteral("RejectAfterTime = %1\n").arg(randomInt(171, 196));
    conf += QStringLiteral("RekeyAfterTime = %1\n").arg(randomInt(116, 131));
    conf += QStringLiteral("RekeyTimeout = %1\n").arg(randomInt(6, 9));

    // AWG 3.1
    conf += QStringLiteral("RandomTrailers = on\n");
    conf += QStringLiteral("DisableCookies = on\n");

    conf += QStringLiteral("\n[Peer]\n");
    conf += QStringLiteral("PublicKey = %1\n").arg(keys.peerPublicKey);
    conf += QStringLiteral("AllowedIPs = 0.0.0.0/0\n");
    conf += QStringLiteral("AllowedIPs = ::/0\n");
    conf += QStringLiteral("Endpoint = %1\n").arg(randomEndpoint());
    conf += QStringLiteral("PersistentKeepalive = 25\n");

    return conf;
}

// --------------------------------------------------------------------------
// (comment translated to English)
// --------------------------------------------------------------------------

QString WarpController::configsPath() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("WarpConfigs"));
}

void WarpController::clearOldConfigs()
{
    QDir dir(configsPath());
    if (!dir.exists()) {
        return;
    }
    const QStringList files = dir.entryList({QStringLiteral("*.conf")}, QDir::Files);
    for (const QString &name : files) {
        dir.remove(name);
    }
}

void WarpController::writeConfigs(const QList<GeneratedConfig> &configs)
{
    const QString path = configsPath();
    QDir().mkpath(path);
    QDir dir(path);

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("HHmmss"));

    m_configs.clear();
    for (int i = 0; i < configs.size(); ++i) {
        const QString fileName = QStringLiteral("warp-%1-%2.conf")
                                     .arg(i + 1, 2, 10, QLatin1Char('0'))
                                     .arg(stamp);
        const QString full = dir.filePath(fileName);

        QFile file(full);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(configs[i].text.toUtf8());
            file.close();
        }

        GeneratedConfig cfg = configs[i];
        cfg.filePath = full;
        m_configs.append(cfg);
    }
}

// --------------------------------------------------------------------------
// (comment translated to English)
// --------------------------------------------------------------------------

void WarpController::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void WarpController::setStatusText(const QString &text)
{
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}

QString WarpController::configText(int index) const
{
    if (index < 0 || index >= m_configs.size()) {
        return {};
    }
    return m_configs[index].text;
}

QString WarpController::configName(int index) const
{
    if (index < 0 || index >= m_configs.size()) {
        return {};
    }
    return m_configs[index].name;
}

void WarpController::copyToClipboard(int index)
{
    if (index < 0 || index >= m_configs.size()) {
        return;
    }
    QGuiApplication::clipboard()->setText(m_configs[index].text);
    setStatusText(tr("Config copied"));
}

void WarpController::openConfigsFolder()
{
    QDir().mkpath(configsPath());
    QDesktopServices::openUrl(QUrl::fromLocalFile(configsPath()));
}

