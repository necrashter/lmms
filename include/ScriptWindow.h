/*
 * ScriptWindow.h - Python scripting window
 */

#ifndef LMMS_GUI_SCRIPT_WINDOW_H
#define LMMS_GUI_SCRIPT_WINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>

#include "SerializingObject.h"

namespace lmms::gui
{

class LMMS_EXPORT ScriptWindow : public QMainWindow, public SerializingObject
{
	Q_OBJECT
public:
	ScriptWindow();
	~ScriptWindow() override = default;

	void clear();
	void setText( const QString & text );

	void saveSettings( QDomDocument & doc, QDomElement & parent ) override;
	void loadSettings( const QDomElement & elem ) override;

	inline QString nodeName() const override
	{
		return "scriptwindow";
	}

protected:
	void closeEvent( QCloseEvent * ce ) override;

private slots:
	void runScript();

private:
	QPlainTextEdit * m_edit;
};

} // namespace lmms::gui

#endif // LMMS_GUI_SCRIPT_WINDOW_H
