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
		//������Ϣ��ʾ
		case QtDebugMsg:
			txt = QString("Debug: %1").arg(msg);
			break;
		//һ���warning��ʾ
		case QtWarningMsg:
			txt = QString("Warning: %1").arg(msg);
			break;
		//���ش�����ʾ
		case QtCriticalMsg:
		     txt = QString("Critical: %1").arg(msg);
		     break;
		//����������ʾ
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

	//��ע���Լ���MsgHandler
	qInstallMsgHandler(CustomMessageHandler);        

	QApplication a(argc, argv);
	CWowModelViewer w;
	w.show();

	w.LoadWOWModelResource();

	return a.exec();
}


// 1, ΪʲôҪ��set, ��Ϊ��Ĭ�������� mpq_libmpq.h--GetFileLists --- ����������㷨����ȷ

// 2, QTreeWidget ֻ��չ�����ڵ� pRoot->setExpanded(true)

// 3, QTreeWidgetItem setData(0, 0) �� setText ��һ����
// Ϊ�������õ�����setdata �� ��ʾ��settext ��һ��, ����setdata(0, 1)
// Ȼ��ȡ��ʱ��data(0, 1) ��Դ�б���ʾ������

// 4, Qt��ȾDX��Ҫ2��...
// a, QPaintEngine *paintEngine() const { return 0; } 
// b, QWidget::setAttribute(Qt::WA_PaintOnScreen);

// 4, CModel Animate ---if (t > a.timeEnd) �������˸������ 
// ��Ϊ t = a.timeEnd ȡ����