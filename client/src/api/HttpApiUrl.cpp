#include "api/HttpApiUrl.h"

#include <QUrlQuery>

namespace HttpApiUrl {

QUrl resolve(const QUrl &baseUrl, const QString &apiPath)
{
    QUrl url = baseUrl;
    if (!url.isValid()) {
        return {};
    }

    const QString normalized = apiPath.startsWith(QLatin1Char('/')) ? apiPath : QStringLiteral("/") + apiPath;
    QString path = url.path();
    if (path.isEmpty() || path == QStringLiteral("/")) {
        url.setPath(normalized);
    } else if (!path.endsWith(normalized)) {
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }
        const QString segment = normalized.startsWith(QLatin1Char('/')) ? normalized.mid(1) : normalized;
        path += segment;
        url.setPath(path);
    }
    return url;
}

QUrl resolve(const QUrl &baseUrl, const QString &apiPath, const QUrlQuery &query)
{
    QUrl url = resolve(baseUrl, apiPath);
    if (url.isValid()) {
        url.setQuery(query);
    }
    return url;
}

} // namespace HttpApiUrl
