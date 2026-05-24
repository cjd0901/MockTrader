#pragma once

#include <QUrl>

class QUrlQuery;

namespace HttpApiUrl {

/// Resolves `baseUrl` + API path (e.g. `/api/stocks`). Appends path segment when base has a prefix.
QUrl resolve(const QUrl &baseUrl, const QString &apiPath);

/// Same as resolve, with query parameters.
QUrl resolve(const QUrl &baseUrl, const QString &apiPath, const QUrlQuery &query);

} // namespace HttpApiUrl
