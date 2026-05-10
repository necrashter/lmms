/*
 * ScriptWindow.cpp - Python scripting window
 */

#include "ScriptWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDomCDATASection>
#include <QPlainTextEdit>
#include <QToolBar>

#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "PythonBindings.h"

namespace lmms::gui
{

ScriptWindow::ScriptWindow()
	: QMainWindow{getGUI()->mainWindow()->workspace()}
{
	m_edit = new QPlainTextEdit(this);
	m_edit->setFont(QFont("Monospace", 10));
	m_edit->show();

	clear();

	QToolBar* tb = addToolBar(tr("Script Actions"));

	auto runAction = new QAction(embed::getIconPixmap("play"), tr("Run script"), this);
	connect(runAction, SIGNAL(triggered()), this, SLOT(runScript()));
	tb->addAction(runAction);

	setCentralWidget(m_edit);
	setWindowTitle(tr("Script Editor"));
	setWindowIcon(embed::getIconPixmap("project_notes"));

	getGUI()->mainWindow()->addWindowedWidget(this);
	parentWidget()->setAttribute(Qt::WA_DeleteOnClose, false);
	parentWidget()->move(700, 10);
	parentWidget()->resize(500, 400);
	parentWidget()->hide();
}

void ScriptWindow::clear()
{
	m_edit->setPlainText(tr("# Write your Python script here\n"));
}

void ScriptWindow::setText(const QString& text)
{
	m_edit->setPlainText(text);
}

void ScriptWindow::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	MainWindow::saveWidgetState(this, parent);
	QDomCDATASection cdataSection = doc.createCDATASection(m_edit->toPlainText());
	parent.appendChild(cdataSection);
}

void ScriptWindow::loadSettings(const QDomElement& elem)
{
	MainWindow::restoreWidgetState(this, elem);
	QDomNode n = elem.firstChild();
	while (!n.isNull())
	{
		if (n.isCDATASection())
		{
			m_edit->setPlainText(n.toCDATASection().data());
			break;
		}
		n = n.nextSibling();
	}
}

void ScriptWindow::closeEvent(QCloseEvent* ce)
{
	if (parentWidget())
		parentWidget()->hide();
	ce->ignore();
}

void ScriptWindow::runScript()
{
	PyImport_AppendInittab("lmms", &PyInit_lmms);
	Py_Initialize();

	const QByteArray code = m_edit->toPlainText().toUtf8();
	PyRun_SimpleString(code.constData());

	Py_Finalize();
}

} // namespace lmms::gui
