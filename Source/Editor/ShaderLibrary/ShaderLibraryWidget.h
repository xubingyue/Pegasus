/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	ShaderLibraryWidget.h
//! \author	Kleber Garcia
//! \date	16 Match 2014
//! \brief	Graphics widget representing the shader library form.

#ifndef EDITOR_SHADERLIBRARYWIDGET_H
#define EDITOR_SHADERLIBRARYWIDGET_H

#include <QDockWidget>
#include "ui_ShaderLibraryWidget.h"

class ProgramTreeModel;

//! Graphics Widget meant for shader navigation & management
class ShaderLibraryWidget : public QDockWidget
{
    Q_OBJECT

public:
    ShaderLibraryWidget(QWidget * parent = 0);
    virtual ~ShaderLibraryWidget();

public slots:
    void UpdateUIForAppLoaded();
    void UpdateUIForAppFinished();
    

private:
    //! Autogenerated ui form from QT designer
    Ui::ShaderLibraryWidget ui;
    ProgramTreeModel * mProgramTreeModel;
};

#endif
