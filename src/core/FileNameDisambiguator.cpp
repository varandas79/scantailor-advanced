// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FileNameDisambiguator.h"
#include <QDomDocument>
#include <QFileInfo>
#include <QMutex>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include "AbstractRelinker.h"
#include "RelinkablePath.h"

using namespace boost::multi_index;

class FileNameDisambiguator::Impl {
 public:
  Impl();

  Impl(const QDomElement& disambiguator_el, const boost::function<QString(const QString&)>& file_path_unpacker);

  QDomElement toXml(QDomDocument& doc,
                    const QString& name,
                    const boost::function<QString(const QString&)>& file_path_packer) const;

  int getLabel(const QString& file_path) const;

  int registerFile(const QString& file_path);

  void performRelinking(const AbstractRelinker& relinker);

 private:
  class ItemsByFilePathTag;
  class ItemsByFileNameLabelTag;

  class UnorderedItemsTag;

  struct Item {
    QString filePath;
    QString fileName;
    int label;

    Item(const QString& file_path, int lbl);

    Item(const QString& file_path, const QString& file_name, int lbl);
  };

  typedef multi_index_container<
      Item,
      indexed_by<
          ordered_unique<tag<ItemsByFilePathTag>, member<Item, QString, &Item::filePath>>,
          ordered_unique<tag<ItemsByFileNameLabelTag>,
                         composite_key<Item, member<Item, QString, &Item::fileName>, member<Item, int, &Item::label>>>,
          sequenced<tag<UnorderedItemsTag>>>>
      Container;

  typedef Container::index<ItemsByFilePathTag>::type ItemsByFilePath;
  typedef Container::index<ItemsByFileNameLabelTag>::type ItemsByFileNameLabel;
  typedef Container::index<UnorderedItemsTag>::type UnorderedItems;

  mutable QMutex m_mutex;
  Container m_items;
  ItemsByFilePath& m_itemsByFilePath;
  ItemsByFileNameLabel& m_itemsByFileNameLabel;
  UnorderedItems& m_unorderedItems;
};


/*====================== FileNameDisambiguator =========================*/

FileNameDisambiguator::FileNameDisambiguator() : m_impl(new Impl) {}

FileNameDisambiguator::FileNameDisambiguator(const QDomElement& disambiguator_el)
    : m_impl(new Impl(disambiguator_el, boost::lambda::_1)) {}

FileNameDisambiguator::FileNameDisambiguator(const QDomElement& disambiguator_el,
                                             const boost::function<QString(const QString&)>& file_path_unpacker)
    : m_impl(new Impl(disambiguator_el, file_path_unpacker)) {}

QDomElement FileNameDisambiguator::toXml(QDomDocument& doc, const QString& name) const {
  return m_impl->toXml(doc, name, boost::lambda::_1);
}

QDomElement FileNameDisambiguator::toXml(QDomDocument& doc,
                                         const QString& name,
                                         const boost::function<QString(const QString&)>& file_path_packer) const {
  return m_impl->toXml(doc, name, file_path_packer);
}

int FileNameDisambiguator::getLabel(const QString& file_path) const {
  return m_impl->getLabel(file_path);
}

int FileNameDisambiguator::registerFile(const QString& file_path) {
  return m_impl->registerFile(file_path);
}

void FileNameDisambiguator::performRelinking(const AbstractRelinker& relinker) {
  m_impl->performRelinking(relinker);
}

/*==================== FileNameDisambiguator::Impl ====================*/

FileNameDisambiguator::Impl::Impl()
    : m_items(),
      m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
      m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
      m_unorderedItems(m_items.get<UnorderedItemsTag>()) {}

FileNameDisambiguator::Impl::Impl(const QDomElement& disambiguator_el,
                                  const boost::function<QString(const QString&)>& file_path_unpacker)
    : m_items(),
      m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
      m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
      m_unorderedItems(m_items.get<UnorderedItemsTag>()) {
  QDomNode node(disambiguator_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != "mapping") {
      continue;
    }
    const QDomElement file_el(node.toElement());

    const QString file_path_shorthand(file_el.attribute("file"));
    const QString file_path = file_path_unpacker(file_path_shorthand);
    if (file_path.isEmpty()) {
      // Unresolved shorthand - skipping this record.
      continue;
    }

    const int label = file_el.attribute("label").toInt();
    m_items.insert(Item(file_path, label));
  }
}

QDomElement FileNameDisambiguator::Impl::toXml(QDomDocument& doc,
                                               const QString& name,
                                               const boost::function<QString(const QString&)>& file_path_packer) const {
  const QMutexLocker locker(&m_mutex);

  QDomElement el(doc.createElement(name));

  for (const Item& item : m_unorderedItems) {
    const QString file_path_shorthand = file_path_packer(item.filePath);
    if (file_path_shorthand.isEmpty()) {
      // Unrepresentable file path - skipping this record.
      continue;
    }

    QDomElement file_el(doc.createElement("mapping"));
    file_el.setAttribute("file", file_path_shorthand);
    file_el.setAttribute("label", item.label);
    el.appendChild(file_el);
  }

  return el;
}

int FileNameDisambiguator::Impl::getLabel(const QString& file_path) const {
  const QMutexLocker locker(&m_mutex);

  const ItemsByFilePath::iterator fp_it(m_itemsByFilePath.find(file_path));
  if (fp_it != m_itemsByFilePath.end()) {
    return fp_it->label;
  }

  return 0;
}

int FileNameDisambiguator::Impl::registerFile(const QString& file_path) {
  const QMutexLocker locker(&m_mutex);

  const ItemsByFilePath::iterator fp_it(m_itemsByFilePath.find(file_path));
  if (fp_it != m_itemsByFilePath.end()) {
    return fp_it->label;
  }

  int label = 0;

  const QString file_name(QFileInfo(file_path).fileName());
  const ItemsByFileNameLabel::iterator fn_it(m_itemsByFileNameLabel.upper_bound(boost::make_tuple(file_name)));
  // If the item preceeding fn_it has the same file name,
  // the new file belongs to the same disambiguation group.
  if (fn_it != m_itemsByFileNameLabel.begin()) {
    ItemsByFileNameLabel::iterator prev(fn_it);
    --prev;
    if (prev->fileName == file_name) {
      label = prev->label + 1;
    }
  }  // Otherwise, label remains 0.
  const Item new_item(file_path, file_name, label);
  m_itemsByFileNameLabel.insert(fn_it, new_item);

  return label;
}

void FileNameDisambiguator::Impl::performRelinking(const AbstractRelinker& relinker) {
  const QMutexLocker locker(&m_mutex);
  Container new_items;

  for (const Item& item : m_unorderedItems) {
    const RelinkablePath old_path(item.filePath, RelinkablePath::File);
    Item new_item(relinker.substitutionPathFor(old_path), item.label);
    new_items.insert(new_item);
  }

  m_items.swap(new_items);
}

/*============================ Impl::Item =============================*/

FileNameDisambiguator::Impl::Item::Item(const QString& file_path, int lbl)
    : filePath(file_path), fileName(QFileInfo(file_path).fileName()), label(lbl) {}

FileNameDisambiguator::Impl::Item::Item(const QString& file_path, const QString& file_name, int lbl)
    : filePath(file_path), fileName(file_name), label(lbl) {}
