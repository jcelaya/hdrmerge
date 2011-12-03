#include <QApplication>
#include "HdrMergeMainWindow.h"

int main(int argc, char * argv[]) {
	QApplication app(argc, argv);

	// Settings
	QCoreApplication::setOrganizationName("JaviSoft");
	QCoreApplication::setOrganizationDomain("javisoft.com");
	QCoreApplication::setApplicationName("HdrMerge");

	MainWindow mw;
	mw.show();

	return app.exec();
}

