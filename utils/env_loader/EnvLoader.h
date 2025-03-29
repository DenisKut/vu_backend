#include <QHash>
#include <QString>

class EnvLoader {
public:
    EnvLoader(const QString& path = ".env");
    bool load();
    QString get(const QString& key, const QString& defaultValue = "") const;
    const QHash<QString, QString>& variables() const { return _variables; }

private:
    QString _path;
    QHash<QString, QString> _variables;
};
