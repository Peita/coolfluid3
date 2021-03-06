// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "ui/core/NTree.hpp"

#include "ui/core/PropertyModel.hpp"

////////////////////////////////////////////////////////////////////////////

namespace cf3 {
namespace ui {
namespace core {

////////////////////////////////////////////////////////////////////////////

PropertyModel::PropertyModel()
  : QAbstractItemModel()
{
  current_index_changed(NTree::global()->current_index(), QModelIndex());

  m_columns << "Name" << "Value";

  connect(NTree::global().get(), SIGNAL(current_index_changed(QModelIndex,QModelIndex)),
          this, SLOT(current_index_changed(QModelIndex,QModelIndex)));
}

////////////////////////////////////////////////////////////////////////////

//PropertyModel::~PropertyModel()
//{
//  this->emptyList();
//}

////////////////////////////////////////////////////////////////////////////

QVariant  PropertyModel::data(const QModelIndex & index, int role) const
{
  QVariant data;

  if(role == Qt::DisplayRole && index.isValid())
  {
    PropertyItem * item = m_data.at(index.row());

    switch(index.column())
    {
    case 0:
      data = item->m_name;
      break;
    case 1:
      data = item->m_value;
      break;
    }
  }

  return data;
}

////////////////////////////////////////////////////////////////////////////

QModelIndex PropertyModel::index(int row, int column, const QModelIndex & parent) const
{
  PropertyItem * item;
  QModelIndex index;


  if(this->hasIndex(row, column, parent))
  {
    item = m_data.at(row);

    if(!parent.isValid())
      index = createIndex(row, column, item);
  }

  return index;
}

////////////////////////////////////////////////////////////////////////////

QModelIndex PropertyModel::parent(const QModelIndex &child) const
{
  return QModelIndex();
}

////////////////////////////////////////////////////////////////////////////

int PropertyModel::rowCount(const QModelIndex & parent) const
{
  if (!parent.isValid())
    return m_data.size();

  return 0;
}

////////////////////////////////////////////////////////////////////////////

int PropertyModel::columnCount(const QModelIndex & parent) const
{
  return m_columns.count();
}

////////////////////////////////////////////////////////////////////////////

QVariant PropertyModel::headerData(int section, Qt::Orientation orientation,
                           int role) const
{
  if(role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0
     && section < m_columns.size())
    return m_columns.at(section);

  return QVariant();
}

////////////////////////////////////////////////////////////////////////////

void PropertyModel::current_index_changed(const QModelIndex & newIndex,
                                        const QModelIndex & oldIndex)
{
  QMap<QString, QString> props;
  QMap<QString, QString>::iterator it;
  unsigned int i;

  emit layoutAboutToBeChanged();

  NTree::global()->list_node_properties(newIndex, props);

  this->empty_list();

  for(i = 0, it = props.begin() ; it != props.end() ; it++, i++)
  {
    m_data << new PropertyItem(it.key(), it.value(), i);
  }

  emit layoutChanged();
}

////////////////////////////////////////////////////////////////////////////

void PropertyModel::empty_list()
{
  while(!m_data.isEmpty())
    delete m_data.takeFirst();
}

////////////////////////////////////////////////////////////////////////////

} // Core
} // ui
} // cf3
