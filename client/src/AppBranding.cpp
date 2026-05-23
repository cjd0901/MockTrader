#include "AppBranding.h"

namespace AppBranding {

namespace {
constexpr const char *kLogoPath = ":/logo.png";
}

QIcon applicationIcon()
{
    const QPixmap pm(kLogoPath);
    if (pm.isNull()) {
        return {};
    }
    return QIcon(pm);
}

QPixmap logoPixmap(int height)
{
    QPixmap pm(kLogoPath);
    if (pm.isNull() || height <= 0) {
        return pm;
    }
    return pm.scaledToHeight(height, Qt::SmoothTransformation);
}

} // namespace AppBranding
