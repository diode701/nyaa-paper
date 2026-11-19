#include "io/ProjectJson.h"

#include <algorithm>
#include <limits>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QString>

namespace nyaa::io {

using nyaa::domain::settings::Kind;
using nyaa::domain::settings::Rgb;
using nyaa::domain::settings::SettingDesc;

namespace {

QString cleanHtml(QString text)
{
    text.replace(QRegularExpression(R"(<br\s*/?>)", QRegularExpression::CaseInsensitiveOption), "\n");
    text.remove(QRegularExpression("<[^>]+>"));
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&quot;", "\"");
    text.replace("&apos;", "'");
    text.replace("&nbsp;", " ");
    text.replace(QRegularExpression("\n{3,}"), "\n\n");
    return text.trimmed();
}

double clamp01(double v)
{
    return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}

Rgb parseColor(const QJsonValue &value)
{
    const QString raw = value.toString().trimmed();

    // "#RRGGBB" or "RRGGBB"
    QString hex = raw;
    if (hex.startsWith('#'))
        hex.remove(0, 1);
    if (hex.size() == 6) {
        bool ok = false;
        const int packed = hex.toInt(&ok, 16);
        if (ok) {
            return {((packed >> 16) & 0xFF) / 255.0,
                    ((packed >> 8) & 0xFF) / 255.0,
                    (packed & 0xFF) / 255.0};
        }
    }

    // "r g b", either 0..1 or 0..255
    const QStringList parts = raw.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() >= 3) {
        bool ok = true;
        double r = parts[0].toDouble(&ok);
        double g = ok ? parts[1].toDouble(&ok) : 0.0;
        double b = ok ? parts[2].toDouble(&ok) : 0.0;
        if (ok) {
            if (r > 1.0 || g > 1.0 || b > 1.0) {
                r /= 255.0;
                g /= 255.0;
                b /= 255.0;
            }
            return {clamp01(r), clamp01(g), clamp01(b)};
        }
    }
    return {};
}

// project.json is hand-written by wallpaper authors, so numbers often arrive
// as strings ("value": "1"). These mirror how Java's PropertyParser read them.

/// PropertyParser.parseDouble: a number, or a string that parses as one.
double toDouble(const QJsonValue &value, double fallback)
{
    if (value.isDouble())
        return value.toDouble();
    if (value.isString()) {
        bool ok = false;
        const double parsed = value.toString().trimmed().toDouble(&ok);
        if (ok)
            return parsed;
    }
    return fallback;
}

/// parseCombo: ((Number) raw).intValue() or Integer.parseInt(raw.toString()).
int toInt(const QJsonValue &value, int fallback)
{
    if (value.isDouble())
        return static_cast<int>(value.toDouble());
    if (value.isString()) {
        bool ok = false;
        const int parsed = value.toString().trimmed().toInt(&ok);
        if (ok)
            return parsed;
    }
    return fallback;
}

/// parseBoolean: Boolean as is, Number != 0, or the strings "true" / "1".
bool toBool(const QJsonValue &value)
{
    if (value.isBool())
        return value.toBool();
    if (value.isDouble())
        return static_cast<int>(value.toDouble()) != 0;
    const QString text = value.toString().trimmed().toLower();
    return text == QLatin1String("true") || text == QLatin1String("1");
}

/// parseTextInput: prop.get("value").toString(), whatever the JSON type.
QString toText(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 15);
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return {};
}

Kind kindFromType(const QString &type)
{
    if (type == "bool" || type == "boolean") return Kind::Bool;
    if (type == "slider") return Kind::Slider;
    if (type == "color") return Kind::Color;
    if (type == "combo") return Kind::Combo;
    if (type == "text") return Kind::Text;
    if (type == "group") return Kind::Group;
    if (type == "textinput") return Kind::TextInput;
    if (type == "file") return Kind::File;
    if (type == "directory") return Kind::Directory;
    if (type == "scenetexture") return Kind::Texture;
    return Kind::Text; // wallpapers that omit "type" are labels
}

bool isKnownType(const QString &type)
{
    static const QSet<QString> known{"bool", "boolean", "slider", "color", "combo", "text",
                                     "group", "textinput", "file", "directory", "scenetexture", ""};
    return known.contains(type);
}

} // namespace

std::vector<SettingDesc> parseProperties(const std::string &json)
{
    std::vector<SettingDesc> out;

    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (!document.isObject())
        return out;

    const QJsonObject general = document.object().value("general").toObject();
    const QJsonObject properties = general.value("properties").toObject();

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const QJsonObject property = it.value().toObject();
        const QString type = property.value("type").toString();
        if (!isKnownType(type))
            continue;

        SettingDesc desc;
        desc.key = it.key().toStdString();
        const QString text = cleanHtml(property.value("text").toString());
        desc.label = text.isEmpty() ? it.key().toStdString() : text.toStdString();
        desc.kind = kindFromType(type);
        desc.order = property.value("order").toInt(std::numeric_limits<int>::max());

        const QJsonValue value = property.value("value");
        bool malformed = false;   // PropertyParser's "Skipping malformed property"
        switch (desc.kind) {
        case Kind::Bool:
            desc.boolean = toBool(value);
            break;
        case Kind::Slider:
            // Java defaults: value 0, min 0, max 100, step 1
            desc.number = toDouble(value, 0.0);
            desc.range.min = toDouble(property.value("min"), 0.0);
            desc.range.max = toDouble(property.value("max"), 100.0);
            desc.range.step = toDouble(property.value("step"), 1.0);
            if (desc.range.step <= 0.0)
                desc.range.step = 1.0;
            break;
        case Kind::Color:
            desc.color = parseColor(value);
            break;
        case Kind::Combo: {
            desc.number = static_cast<double>(toInt(value, 0));
            const QJsonArray options = property.value("options").toArray();
            for (const QJsonValue &option : options) {
                const QJsonObject object = option.toObject();
                // item.optString("label", item.optString("text", "?"))
                QString label = object.value("label").toString();
                if (label.isEmpty())
                    label = object.value("text").toString(QStringLiteral("?"));
                // Java: Integer.parseInt throws on "dark1"/"en", and
                // parseFromWppr skips the whole property. Same here.
                constexpr int kNotANumber = std::numeric_limits<int>::min();
                const int key = toInt(object.value("value"), kNotANumber);
                if (key == kNotANumber) {
                    malformed = true;
                    break;
                }
                desc.options.emplace_back(key, label.toStdString());
            }
            break;
        }
        case Kind::Text:
        case Kind::Group:
            break;
        default:
            // textinput, file, directory, scenetexture
            desc.text = toText(value).toStdString();
            break;
        }

        if (malformed)
            continue;
        out.push_back(std::move(desc));
    }

    std::stable_sort(out.begin(), out.end(),
                     [](const SettingDesc &a, const SettingDesc &b) { return a.order < b.order; });
    return out;
}

std::vector<SettingDesc> parseProjectFile(const std::string &path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return parseProperties(file.readAll().toStdString());
}

} // namespace nyaa::io
