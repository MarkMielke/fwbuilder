/*

                          Firewall Builder

                 Copyright (C) 2003, 2006 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../../config.h"
#include "global.h"

#include "FWWindow.h"
#include "ProjectPanel.h"
#include "ObjectManipulator.h"
#include "FWObjectClipboard.h"
#include "FWBTree.h"
#include "FWBSettings.h"
#include "RuleSetView.h"
#include "FindObjectWidget.h"
#include "FindWhereUsedWidget.h"

#include "events.h"

#include "fwbuilder/FWReference.h"
#include "fwbuilder/Tools.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/FWException.h"
#include "fwbuilder/Library.h"
#include "fwbuilder/FWObject.h"

/*
#include <qaction.h>
#include <qlistwidget.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qheaderview.h>
#include <qtabwidget.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtextedit.h>
#include <qstringlist.h>
#include <qmenu.h>
#include <qtoolbutton.h>

#include <qlayout.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qsplitter.h>
#include <qtimer.h>
#include <qstatusbar.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qprinter.h>
#include <qstackedwidget.h>
#include <qlistwidget.h>
#include <qeventloop.h>
#include <qtextstream.h>
#include <QCloseEvent>
#include <QShowEvent>
#include <QList>
#include <QHideEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSignalMapper>
#include <QUrl>
*/

/*
 *  Methods in this module are just wrappers and call the same method
 *  in the topmost object of the ProjectPanel class (MDI subwindow).
 *
 * TODO: see if most of these methods can be called directly using
 * pointer to the ProjectPanel object. Remove most, if not all,
 * wrappers in the FWWindow class.
 */


using namespace libfwbuilder;
using namespace std;
using namespace Ui;



/*************************************************************************/


bool FWWindow::saveIfModified()
{
    if (activeProject()) return activeProject()->saveIfModified();
    return false;
}

QString FWWindow::getDestDir(const QString &fname)
{
   if (activeProject()) return activeProject()->getDestDir(fname);
   return "";
}

QString FWWindow::chooseNewFileName(const QString &fname, const QString &title)
{  
    if (activeProject()) return activeProject()->chooseNewFileName(fname,title);
    return "";
}

void FWWindow::setFileName(const QString &fname)
{
    if (activeProject()) activeProject()->setFileName(fname);
}

void FWWindow::fileProp()
{
    if (activeProject()) activeProject()->fileProp();
}

void FWWindow::fileSave()
{
    if (activeProject()) activeProject()->fileSave();
}

void FWWindow::fileSaveAs()
{
    if (activeProject()) activeProject()->fileSaveAs();
}

void FWWindow::fileCommit()
{
    if (activeProject()) activeProject()->fileCommit();
}

/*
 * discard changes done to the file and check out clean copy of the
 * head revision from RCS
 */
void FWWindow::fileDiscard()
{
    if (activeProject()) activeProject()->fileDiscard();
}

void FWWindow::fileAddToRCS()
{
    if (activeProject()) activeProject()->fileAddToRCS();
}

void FWWindow::load(QWidget*)
{
    if (activeProject()) activeProject()->loadStandardObjects();
}

void FWWindow::load(QWidget*, RCS *_rcs)
{
    if (activeProject()) activeProject()->loadFromRCS(_rcs);
}

void FWWindow::save()
{
    if (activeProject()) activeProject()->save();
}

void FWWindow::loadLibrary(const string &libfpath)
{
    if (activeProject()) activeProject()->loadLibrary(libfpath);
}

void FWWindow::fileImport()
{
    if (activeProject()) activeProject()->fileImport();
}


void FWWindow::fileCompare()
{
    if (activeProject()) activeProject()->fileCompare();
}

void FWWindow::findExternalRefs(FWObject *lib,
                                FWObject *root,
                                list<FWReference*> &extRefs)
{
    if (activeProject()) activeProject()->findExternalRefs(lib, root, extRefs);
}

bool FWWindow::checkin(bool unlock)
{
    if (activeProject()) return activeProject()->checkin(unlock);
    return false;
}

void FWWindow::setSafeMode(bool f)
{
    if (activeProject()) activeProject()->setSafeMode(f);
}

bool FWWindow::exportLibraryTest(list<FWObject*> &selectedLibs)
{
/* VERY IMPORTANT: External library file must be self-contained,
 * otherwise it can not be exported.
 *
 * check if selected libraries have references to objects in other
 * libraries (not exported to the same file). Exporting such libraries
 * pulls in other ones because of these references. This is confusing
 * because it means we end up with multiple copies of such objects (in
 * exported library file and in user's data file). When user imports
 * this library and opens their file, it is impossible to say which
 * library an object belongs to.
 *
 * This is prohibited. We check if exported set of libraries has such
 * references and refuse to export it. The user is supposed to clean
 * it up by either moving objects into the library they are trying to
 * export, or by rearranging objects. The only exception for this is
 * library "Standard", which is assumed to be always present so we can
 * have references to objects in it.
 */
    if (activeProject())
        return activeProject()->exportLibraryTest(selectedLibs);
    return false;
}

void FWWindow::exportLibraryTo(QString fname,list<FWObject*> &selectedLibs, bool rof)
{
    if (activeProject())
        activeProject()->exportLibraryTo(fname,selectedLibs, rof);
}

void FWWindow::fileExport()
{
    if (activeProject()) activeProject()->fileExport();
}

int  FWWindow::findFirewallInList(FWObject *f)
{
    if (activeProject()) return activeProject()->findFirewallInList(f);
    return -1;
}

void FWWindow::editCopy()
{
    if (activeProject()) activeProject()->editCopy();
}

void FWWindow::editCut()
{
    if (activeProject())
    {
        activeProject()->editCut();
    }
}

void FWWindow::editDelete()
{
    if (activeProject())
    {
        activeProject()->editDelete();
    }
}

void FWWindow::editPaste()
{
    if (activeProject())
    {
        activeProject()->editPaste();
    }
}

void FWWindow::toggleViewObjectTree()
{
    if (activeProject())
    {
        activeProject()->toggleViewTree(m_mainWindow->actionObject_Tree->isChecked());
    }
}

void FWWindow::toggleViewRules()
{
    if (activeProject())
    {
        activeProject()->toggleViewRules(m_mainWindow->actionRules->isChecked());
    }
}

void FWWindow::toggleViewEditor()
{
    if (activeProject())
    {
        activeProject()->toggleViewEditor(m_mainWindow->actionEditor_panel->isChecked());
    }
}

void FWWindow::search()
{
    if (activeProject())
    {
        m_mainWindow->actionEditor_panel->setChecked(true);
        // call ProjectPanel::search() to actually open search panel
        // and reset it to normal search box rather than "find where
        // used"
        activeProject()->search();
    }
}

void FWWindow::insertRule()
{
    if (activeProject()) activeProject()->insertRule();
}

void FWWindow::addRuleAfterCurrent()
{
    if (activeProject())
        activeProject()->addRuleAfterCurrent();
}

void FWWindow::removeRule()
{
    if (activeProject()) activeProject()->removeRule();
}

void FWWindow::moveRule()
{
    if (activeProject()) activeProject()->moveRule();
}

void FWWindow::moveRuleUp()
{
    if (activeProject()) activeProject()->moveRuleUp();
}

void FWWindow::moveRuleDown()
{
    if (activeProject()) activeProject()->moveRuleDown();
}

void FWWindow::copyRule()
{
    if (activeProject()) activeProject()->copyRule();
}

void FWWindow::cutRule()
{
    if (activeProject()) activeProject()->cutRule();
}

void FWWindow::pasteRuleAbove()
{
    if (activeProject()) activeProject()->pasteRuleAbove();
}

void FWWindow::pasteRuleBelow()
{
    if (activeProject()) activeProject()->pasteRuleBelow();
}

void FWWindow::findWhereUsed(FWObject * obj)
{
    if (activeProject()) activeProject()->findWhereUsed(obj);
}

void FWWindow::back()
{
    if (activeProject()) activeProject()->back();
}

void FWWindow::newObject()
{
    if (activeProject()) activeProject()->newObject();
}

// ObjectManipulator::lockObject calls
// mw->reloadAllWindowsWithFile(activeProject()) to update
// other windows
void FWWindow::lockObject()
{
    if (activeProject()) activeProject()->lockObject();
}

// ObjectManipulator::unlockObject calls
// mw->reloadAllWindowsWithFile(activeProject()) to update
// other windows
void FWWindow::unlockObject()
{
    if (activeProject()) activeProject()->unlockObject();
}

void FWWindow::setupAutoSave()
{
    if (activeProject()) activeProject()->setupAutoSave();
}

QString FWWindow::getCurrentFileName()
{
    if (activeProject()) return activeProject()->getCurrentFileName();
    return "";
}

RCS * FWWindow::getRCS()
{
    if (activeProject()) return activeProject()->getRCS();
    return 0;
}

void FWWindow::findObject(FWObject *o)
{
    if (activeProject()) activeProject()->findObject(o);
}

/*
 * reset tab via callback because calling setCurrentPage from
 * ruleSetTabChanged causes recursive call to ruleSetTabChanged
 */
void FWWindow::restoreRuleSetTab()
{
    if (activeProject()) activeProject()->restoreRuleSetTab();
}

FWObject* FWWindow::getCurrentLib()
{
    if (activeProject())
        return activeProject()->getCurrentLib();
    return 0;
}

void FWWindow::loadDataFromFw(Firewall *fw)
{
    if (activeProject())
        activeProject()->loadDataFromFw(fw);
}

void FWWindow::insertObjectInTree(FWObject *parent, FWObject *obj)
{
    if (activeProject())
        activeProject()->insertObjectInTree(parent, obj);
}

FWObject* FWWindow::createObject(const QString &objType,
                                               const QString &objName,
                                               FWObject *copyFrom)
{
    FWObject *res = NULL;
    if (activeProject())
    {
        res = activeProject()->createObject(objType, objName, copyFrom);
    }
    return res;
}

FWObject* FWWindow::createObject(FWObject *parent,
                                               const QString &objType,
                                               const QString &objName,
                                               FWObject *copyFrom)
{
    FWObject *res = NULL;
    if (activeProject())
    {
        res =  activeProject()->createObject(parent, objType,
                                             objName, copyFrom);
    }
    return res;
}

void FWWindow::moveObject(FWObject *target, FWObject *obj)
{
    if (activeProject())
    {
        activeProject()->moveObject(target, obj);
    }
}

void FWWindow::moveObject(const QString &targetLibName, FWObject *obj)
{
    if (activeProject())
    {
        activeProject()->moveObject(targetLibName, obj);
    }
}

FWObject* FWWindow::pasteTo(FWObject *target, FWObject *obj)
{
    if (activeProject()) return activeProject()->pasteTo(target, obj);
    return 0;
}

void FWWindow::delObj(FWObject *obj,bool openobj)
{
    if (activeProject())
    {
        activeProject()->delObj(obj, openobj);
    }
}

ObjectTreeView* FWWindow::getCurrentObjectTree()
{
    if (activeProject()) return activeProject()->getCurrentObjectTree();
    return 0;
}

void FWWindow::findAllFirewalls (std::list<Firewall *> &fws)
{
    if (activeProject()) activeProject()->findAllFirewalls (fws);
}

FWObject* FWWindow::duplicateObject(FWObject *target,
                                    FWObject *obj,
                                    const QString &name,
                                    bool  askForAutorename)
{
    if (activeProject())
        return activeProject()->duplicateObject(target,
                                                obj, name, askForAutorename);
    return 0;
}

void FWWindow::showDeletedObjects(bool f)
{
    if (activeProject()) activeProject()->showDeletedObjects(f);
}
/*
void FWWindow::select()
{
    if (activeProject()) activeProject()->select();
}

void FWWindow::unselect()
{
    if (activeProject()) activeProject()->unselect();
}
*/

FWObjectDatabase* FWWindow::db() 
{ 
    if (activeProject()) return activeProject()->db(); 
    return NULL;
}

QString FWWindow::printHeader()
{
    if (activeProject()) return activeProject()->printHeader();
    return "";
}

bool FWWindow::editingLibrary()
{
    if (activeProject()) return activeProject()->editingLibrary();
    return false;
}

