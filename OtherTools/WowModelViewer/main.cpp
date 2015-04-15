#include "wowmodelviewer.h"
#include <QtGui/QApplication>
#include <QTextCodec>
#include <QFile>
#include <QTextStream>

void CustomMessageHandler(QtMsgType type, const char *msg)
{
	QString txt;
    switch (type)
	{
		//调试信息提示
		case QtDebugMsg:
			txt = QString("Debug: %1").arg(msg);
			break;
		//一般的warning提示
		case QtWarningMsg:
			txt = QString("Warning: %1").arg(msg);
			break;
		//严重错误提示
		case QtCriticalMsg:
		     txt = QString("Critical: %1").arg(msg);
		     break;
		//致命错误提示
		case QtFatalMsg:
		     txt = QString("Fatal: %1").arg(msg);
		   abort();
	}

	QFile outFile("WOWModelViewer.log");
	outFile.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream ts(&outFile);
	ts << txt << endl;
}


int main(int argc, char *argv[])
{
	QTextCodec::setCodecForTr( QTextCodec::codecForName("GBK") );

	//先注册自己的MsgHandler
	qInstallMsgHandler(CustomMessageHandler);        

	QApplication a(argc, argv);
	CWowModelViewer w;
	w.show();

	w.LoadWOWModelResource();

	return a.exec();
}


// 1, 为什么要用set, 因为有默认排序功能 mpq_libmpq.h--GetFileLists --- 后面的排序算法才正确

// 2, QTreeWidget 只想展开根节点 pRoot->setExpanded(true)

// 3, QTreeWidgetItem setData(0, 0) 和 setText 是一样的
// 为了是设置的数据setdata 和 显示的settext 不一样, 可以setdata(0, 1)
// 然后取的时候data(0, 1) 资源列表显示的问题

// 4, Qt渲染DX需要2点...
// a, QPaintEngine *paintEngine() const { return 0; } 
// b, QWidget::setAttribute(Qt::WA_PaintOnScreen);

// 4, CModel Animate ---if (t > a.timeEnd) 会造成闪烁的问题 
// 因为 t = a.timeEnd 取不到