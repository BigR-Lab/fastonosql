#include "gui/keys_table_model.h"

#include "common/qt/utils_qt.h"

#include "gui/gui_factory.h"

#include "translations/global.h"

namespace fastonosql
{
    KeyTableItem::KeyTableItem(const NDbValue &key)
        : key_(key)
    {

    }

    QString KeyTableItem::key() const
    {
        return common::convertFromString<QString>(key_.keyString());
    }

    QString KeyTableItem::typeText() const
    {
        return common::convertFromString<QString>(common::Value::toString(key_.type()));
    }

    int32_t KeyTableItem::msecTTL() const
    {
        NKey key = key_.key();
        return key.ttl_msec_;
    }

    common::Value::Type KeyTableItem::type() const
    {
        return key_.type();
    }

    KeysTableModel::KeysTableModel(QObject* parent)
        : TableModel(parent)
    {

    }

    KeysTableModel::~KeysTableModel()
    {

    }

    QVariant KeysTableModel::data(const QModelIndex& index, int role) const
    {
        QVariant result;

        if (!index.isValid())
            return result;

        KeyTableItem *node = common::utils_qt::item<KeyTableItem*>(index);

        if (!node)
            return result;

        int col = index.column();

        if(role == Qt::DecorationRole && col == KeyTableItem::kKey){
            return GuiFactory::instance().icon(node->type());
        }

        if(role == Qt::TextColorRole && col == KeyTableItem::kType){
            return QColor(Qt::gray);
        }

        if (role == Qt::DisplayRole) {
            if (col == KeyTableItem::kKey) {
                result = node->key();
            }
            else if (col == KeyTableItem::kType) {
                result = node->typeText();
            }
            else if (col == KeyTableItem::kTTL) {
                result = node->msecTTL();
            }
        }
        return result;
    }

    QVariant KeysTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        using namespace translations;
        if (role != Qt::DisplayRole)
            return QVariant();

        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (section == KeyTableItem::kKey) {
                return trKey;
            }
            else if (section == KeyTableItem::kType) {
                return trType;
            }
            else if (section == KeyTableItem::kTTL) {
                return trMsecTTL;
            }
        }

        return TableModel::headerData(section,orientation,role);
    }

    int KeysTableModel::columnCount(const QModelIndex &parent) const
    {
        return KeyTableItem::kCountColumns;
    }

    void KeysTableModel::clear()
    {
        beginResetModel();
        for(int i = 0; i < data_.size(); ++i){
            delete data_[i];
        }
        data_.clear();
        endResetModel();
    }
}
