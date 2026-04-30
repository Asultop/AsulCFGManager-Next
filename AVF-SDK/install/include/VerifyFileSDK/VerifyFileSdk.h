#pragma once

#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QtCore/QtGlobal>

#include <optional>

#if defined(VERIFYFILE_SDK_STATIC)
#  define VERIFYFILE_SDK_EXPORT
#elif defined(VERIFYFILE_SDK_LIBRARY)
#  define VERIFYFILE_SDK_EXPORT Q_DECL_EXPORT
#else
#  define VERIFYFILE_SDK_EXPORT Q_DECL_IMPORT
#endif

namespace VerifyFile {

enum class VerifyStatus {
    Valid,
    NoSignature,
    InvalidFormat,
    InvalidSignature,
    UnsupportedAlgorithm,
    IoError,
    ChainValidationFailed,
    Revoked,
    FileBindingMismatch
};

struct SignatureInfo {
    QString algorithm;
    QString signer;
    QString keyId;
    QString issuedAtUtc;
    QString payloadHashBase64;
    QString fileUniqueId;
    QString signatureBase64;
    QString leafCertificatePem;
    QStringList intermediateCertificatePems;
    QString certificateSerialHex;
    QString certificateSubject;
    QString certificateIssuer;
    QString certificateFingerprintSha256;
    bool chainTrusted = false;
    bool revocationChecked = false;
    bool revoked = false;
    bool validFormat = false;
    QString errorMessage;
};

struct SigningOptions {
    QString keyId;
    QString algorithm = QStringLiteral("RSA-SHA256");
    QString privateKeyPemPath;
    QString leafCertificatePemPath;
    QStringList intermediateCertificatePemPaths;
};

struct VerificationOptions {
    bool enableChainValidation = true;
    QString rootCaPemPath;
    QStringList extraIntermediatePemPaths;
    bool enableCrlCheck = false;
    QStringList crlPemPaths;
    bool enableOcspCheck = false;
    QString ocspResponseDerPath;
};

struct SimpleVerifyResult {
    bool hasSignature = false;
    bool signatureValid = false;
    bool trusted = false;
    VerifyStatus status = VerifyStatus::NoSignature;
    SignatureInfo info;
    QString errorMessage;
};

class VERIFYFILE_SDK_EXPORT VerifyFileSdk final {
public:
    static QByteArray issueSignatureAsymmetric(
        const QByteArray &payload,
        const SigningOptions &options,
        const QString &fileUniqueId = QString(),
        QString *errorMessage = nullptr
    );

    static std::optional<SignatureInfo> readSignatureInfo(
        const QByteArray &signatureBlob,
        QString *errorMessage = nullptr
    );

    static std::optional<SignatureInfo> readFileSignatureInfo(
        const QString &filePath,
        QString *errorMessage = nullptr
    );

    static bool addSignatureToFileAsymmetric(
        const QString &filePath,
        const SigningOptions &options,
        QByteArray *outSignatureBlob = nullptr,
        QString *errorMessage = nullptr
    );

    static VerifyStatus verifyFileSignatureAsymmetric(
        const QString &filePath,
        const VerificationOptions &options,
        SignatureInfo *outInfo = nullptr,
        QByteArray *outSignatureValue = nullptr,
        QString *errorMessage = nullptr
    );

    static bool hasSignature(
        const QString &filePath,
        bool *outHasSignature = nullptr,
        QString *errorMessage = nullptr
    );

    static bool isFileSignatureValidAsymmetric(
        const QString &filePath,
        const VerificationOptions &options,
        QString *errorMessage = nullptr
    );

    // Minimal API for single-root-CA trust model.
    static bool signFileSimple(
        const QString &filePath,
        const QString &leafPrivateKeyPemPath,
        const QString &leafCertificatePemPath,
        const QString &keyId = QString(),
        const QString &algorithm = QStringLiteral("RSA-SHA256"),
        QString *errorMessage = nullptr
    );

    static SimpleVerifyResult verifyFileSimple(
        const QString &filePath,
        const QString &rootCaPemPath,
        bool enableCrlCheck = false,
        const QStringList &crlPemPaths = {},
        bool enableOcspCheck = false,
        const QString &ocspResponseDerPath = QString(),
        bool enableFileBindingCheck = false
    );

    static bool signFileDetachedSimple(
        const QString &filePath,
        const QString &leafPrivateKeyPemPath,
        const QString &leafCertificatePemPath,
        const QString &keyId = QString(),
        const QString &algorithm = QStringLiteral("RSA-SHA256"),
        QString *errorMessage = nullptr
    );

    static SimpleVerifyResult verifyFileDetachedSimple(
        const QString &filePath,
        const QString &sigFilePath,
        const QString &rootCaPemPath,
        bool enableCrlCheck = false,
        const QStringList &crlPemPaths = {},
        bool enableOcspCheck = false,
        const QString &ocspResponseDerPath = QString()
    );

private:
    VerifyFileSdk() = delete;
};

} // namespace VerifyFile
