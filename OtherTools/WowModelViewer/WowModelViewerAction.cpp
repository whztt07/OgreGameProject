
#include "wowmodelviewer.h"
#include <qtreewidget>

void CWowModelViewer::TouchWOWModelEvent()
{
	connect(m_pWOWResDirTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), 
				this, SLOT(ResDirClickedEvent(QTreeWidgetItem*, int)));
}

void CWowModelViewer::ResDirClickedEvent(QTreeWidgetItem *item, int)
{
	if (item == NULL)
	{
		return;
	}

	//QString name = item->text(0);
	QString path = item->data(0, 1).toString();
	QString right = path.right(1);
	bool bModel = (right.contains(QString('2'), Qt::CaseInsensitive));

	if (bModel)
	{
		LoadModel(path);
	}
}