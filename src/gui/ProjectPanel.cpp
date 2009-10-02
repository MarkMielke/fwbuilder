/*

                          Firewall Builder

                 Copyright (C) 2008 NetCitadel, LLC

  Author:  alek@codeminders.com
           refactoring and bugfixes: vadim@fwbuilder.org

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
#include "utils.h"
#include "utils_no_qt.h"

#include <fwbuilder/Cluster.h>
#include <fwbuilder/Firewall.h>
#include <fwbuilder/RuleSet.h>
#include <fwbuilder/Policy.h>
#include <fwbuilder/NAT.h>
#include <fwbuilder/Routing.h>
#include "fwbuilder/RuleSet.h"
#include "fwbuilder/Rule.h"
#include "fwbuilder/RuleElement.h"

#include "FWBSettings.h"
#include "FWBTree.h"
#include "FWObjectPropertiesFactory.h"
#include "FWWindow.h"
#include "FindObjectWidget.h"
#include "FindWhereUsedWidget.h"
#include "ProjectPanel.h"
#include "RCS.h"
#include "RuleSetView.h"
#include "findDialog.h"
#include "events.h"
#include "ObjectTreeView.h"

#include <QtDebug>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QTimer>
#include <QStatusBar>
#include <QFileInfo>


using namespace Ui;
using namespace libfwbuilder;
using namespace std;



void ProjectPanel::initMain(FWWindow *main)
{
    mainW = main;
    closing = false ;
    mdiWindow = NULL;

    // mdiWindow changes state several times right after it is opened,
    // but we call saveState to store splitter position and its geometry
    // when state changes. Flag "ready" is false after ProjectPanel is created
    // and until FWWindow decides that ProjectPanel is ready for operation.
    // Do not load or save state if flag ready is false.
    ready = false;

    int total_width = DEFAULT_H_SPLITTER_POSITION;
    int total_height = DEFAULT_V_SPLITTER_POSITION;

    if (mainW)
    {
        total_width = mainW->width();
        total_height = mainW->height();
    }

    setMainSplitterPosition(DEFAULT_H_SPLITTER_POSITION,
                            total_width - DEFAULT_H_SPLITTER_POSITION);

    enableAvtoSaveState=true ;
    oldState=-1;

    findObjectWidget = new FindObjectWidget(
        m_panel->auxiliaryPanel, this, "findObjectWidget");
    findObjectWidget->setFocusPolicy( Qt::NoFocus );
    m_panel->auxiliaryPanel->layout()->addWidget( findObjectWidget );

    findWhereUsedWidget = new FindWhereUsedWidget(
        m_panel->auxiliaryPanel, this, "findWhereUsedWidget");
    findWhereUsedWidget->setFocusPolicy( Qt::NoFocus );
    m_panel->auxiliaryPanel->layout()->addWidget( findWhereUsedWidget );
    findWhereUsedWidget->hide();

    m_panel->bottomDockWidget->hide();

    oe  = new ObjectEditor((QWidget*)m_panel->objectEditorStack, this);
    //oe->setCloseButton(m_panel->closeObjectEditorButton);
    oe->setApplyButton(m_panel->applyObjectEditorButton);
    oe->setHelpButton(m_panel->helpObjectEditorButton);
    m_panel->bottomDockWidget->setupEditor(oe);
    closeEditorPanel();

    connect(m_panel->treeDockWidget, SIGNAL(topLevelChanged(bool)),
            this, SLOT(topLevelChangedForTreePanel(bool)));
    connect(m_panel->treeDockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(visibilityChangedForTreePanel(bool)));

    connect(m_panel->bottomDockWidget, SIGNAL(topLevelChanged(bool)),
            this, SLOT(topLevelChangedForBottomPanel(bool)));

    fd  = new findDialog(this, this);
    fd->hide();
}

ProjectPanel::ProjectPanel(QWidget *parent): 
    QWidget(parent), // , Qt::WindowSystemMenuHint|Qt::Window),
    mainW(0),
    rcs(0),
    objectTreeFormat(new FWBTree),
    systemFile(true),
    safeMode(false),
    editingStandardLib(false),
    editingTemplateLib(false),
    ruleSetRedrawPending(false),
    objdb(0),
    editorOwner(0), 
    oe(0),
    fd(0),
    autosaveTimer(new QTimer(static_cast<QObject*>(this))), ruleSetTabIndex(0),
    visibleFirewall(0),
    visibleRuleSet(0),
    lastFirewallIdx(-2),
    changingTabs(false),
    noFirewalls(tr("No firewalls defined")),
    m_panel(0),
    findObjectWidget(0), 
    findWhereUsedWidget(0)
{
    if (fwbdebug) qDebug("ProjectPanel constructor");
    m_panel = new Ui::ProjectPanel_q();
    m_panel->setupUi(this);
    m_panel->om->setupProject(this);

    setWindowTitle(getPageTitle());

    if (fwbdebug) qDebug("New ProjectPanel  %p", this);
}

ProjectPanel::~ProjectPanel()
{
    if (rcs) delete rcs;
    delete m_panel;
}

QString ProjectPanel::getPageTitle()
{
    QString default_caption = tr("Untitled");
    if (rcs)
    {
        QString caption = rcs->getFileName().section("/",-1,-1);
        if (rcs->isInRCS()) caption= caption + ", rev " + rcs->getSelectedRev();
        if (rcs->isRO()) caption = caption + " " + tr("(read-only)");
        if (caption.isEmpty()) return default_caption;
        return caption;
    }
    else return default_caption;
}

RuleElement* ProjectPanel::getRE(Rule* r, int col )
{
    string ret;
    switch (col)
    {
        case 0: ret=RuleElementSrc::TYPENAME; break;//Object
        case 1: ret=RuleElementDst::TYPENAME; break;//Object
        case 2: ret=RuleElementSrv::TYPENAME; break;//Object
        case 3: ret=RuleElementItf::TYPENAME; break;//Object
        case 4: ret=RuleElementInterval::TYPENAME; break;//Time
        default: return NULL;
    }

    return RuleElement::cast( r->getFirstByType(ret) );
}

void ProjectPanel::restoreRuleSetTab()
{
    if (fwbdebug) qDebug("ProjectPanel::()");
    m_panel->ruleSets->setCurrentIndex(ruleSetTabIndex);

}

void ProjectPanel::loadObjects()
{
    m_panel->om->loadObjects();
}

void ProjectPanel::loadObjects(FWObjectDatabase*)
{
    m_panel->om->loadObjects();
}

void ProjectPanel::clearObjects()
{
    m_panel->om->clearObjects();
}

void ProjectPanel::clearFirewallTabs()
{
    if (fwbdebug) qDebug("ProjectPanel::clearFirewallTabs");

    m_panel->ruleSets->hide();

    while (m_panel->ruleSets->count()!=0)
    {
        QWidget *p = m_panel->ruleSets->widget(0);
        m_panel->ruleSets->removeWidget(
            m_panel->ruleSets->widget(m_panel->ruleSets->indexOf(p)));
        delete p;
    }
    m_panel->rulesetname->setText("");
    m_panel->ruleSets->show();
    ruleSetViews.clear();
}

void ProjectPanel::ensureObjectVisibleInRules(FWReference *obj)
{
    if (fwbdebug) qDebug("ProjectPanel::ensureObjectVisibleInRules");
    FWObject *p=obj;
    while (p && RuleSet::cast(p)==NULL ) p=p->getParent();
    if (p==NULL) return;  // something is broken
    openRuleSet(p);
    getCurrentRuleSetView()->setFocus();
    getCurrentRuleSetView()->selectRE( obj );    
}

RuleSetView * ProjectPanel::getCurrentRuleSetView () 
{
    return dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget ());
}


void ProjectPanel::reopenFirewall()
{
    if (fwbdebug)  qDebug("ProjectPanel::reopenFirewall()");

    time_t last_modified = db()->getTimeLastModified();
    if (fwbdebug)
        qDebug("ProjectPanel::reopenFirewall(): checkpoint 1: "
               "dirty=%d last_modified=%s",
               db()->isDirty(), ctime(&last_modified));

    if (ruleSetRedrawPending) return;

    int currentPage = m_panel->ruleSets->currentIndex();

    SelectionMemento memento;

    RuleSetView* rv = dynamic_cast<RuleSetView*>(
        m_panel->ruleSets->currentWidget());
    if (rv) rv->saveCurrentRowColumn(memento);

    last_modified = db()->getTimeLastModified();
    if (fwbdebug)
        qDebug("ProjectPanel::reopenFirewall(): checkpoint 2: "
               "dirty=%d last_modified=%s",
               db()->isDirty(), ctime(&last_modified));

    // since reopenFirewall deletes and recreates all RuleSetView
    // widgets, it causes significant amount of repaint and
    // flicker. Disable updates for the duration of operation to avoid
    // that.
    m_panel->ruleSets->setUpdatesEnabled(false);

    changingTabs = true;

    QStatusBar *sb = mainW->statusBar();
    sb->showMessage( tr("Building policy view...") );
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);

    clearFirewallTabs();

    last_modified = db()->getTimeLastModified();
    if (fwbdebug)
        qDebug("ProjectPanel::reopenFirewall(): checkpoint 3: "
               "dirty=%d last_modified=%s",
               db()->isDirty(), ctime(&last_modified));
    
    if (visibleRuleSet==NULL) return ;

    for (int i =0 ; i < m_panel->ruleSets->count (); i++)
        m_panel->ruleSets->removeWidget(m_panel->ruleSets->widget(i));

    m_panel->rulesetname->setTextFormat(Qt::RichText);
    updateFirewallName();

    last_modified = db()->getTimeLastModified();
    if (fwbdebug)
        qDebug("ProjectPanel::reopenFirewall(): checkpoint 4: "
               "dirty=%d last_modified=%s",
               db()->isDirty(), ctime(&last_modified));
    
    m_panel->ruleSets->addWidget(RuleSetView::getRuleSetViewByType(this, visibleRuleSet,NULL));

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);

    last_modified = db()->getTimeLastModified();
    if (fwbdebug)
        qDebug("ProjectPanel::reopenFirewall(): checkpoint 5: "
               "dirty=%d last_modified=%s",
               db()->isDirty(), ctime(&last_modified));
    
    m_panel->ruleSets->setCurrentIndex( currentPage );
    rv = dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget());
    rv->restoreCurrentRowColumn(memento);
    
    sb->clearMessage();

    changingTabs = false;
    mainW->setEnabledAfterRF();

    m_panel->ruleSets->setUpdatesEnabled(true);
    m_panel->ruleSets->show();
}

int  ProjectPanel::findFirewallInList(FWObject *f)
{
    vector<FWObject*>::iterator i;
    int n=0;
    for (i=firewalls.begin(); i!=firewalls.end(); i++,n++)
    {
        if ( (*i)->getId()==f->getId() ) return n;
    }
    return -1;
}

void ProjectPanel::updateFirewallName()
{
    if (visibleRuleSet==NULL)
        return ;

    QString name = "<B>";
    FWObject * p = visibleRuleSet->getParent();
    name += QString::fromUtf8(p->getName().c_str());
    name += " / ";
    name += QString::fromUtf8(visibleRuleSet->getName().c_str());
    name += "</B>";
    m_panel->rulesetname->setText(name );
}

void ProjectPanel::openRuleSet(FWObject * obj)
{
    blankEditor();
    RuleSet * rs = RuleSet::cast(obj);
    if (rs!= visibleRuleSet)
    {
        visibleRuleSet = rs;
        scheduleRuleSetRedraw();
    }
}

void ProjectPanel::selectRules()
{
//    `unselect();
    RuleSetView* rv = dynamic_cast<RuleSetView*>(
        m_panel->ruleSets->currentWidget());
    rv->setFocus();
}

void ProjectPanel::unselectRules()
{
    bool havePolicies = (m_panel->ruleSets->count()!=0);

/* commented this out so that when I hit "Edit" in the object's pop-down
 * menu in a rule, ruleset wont lose focus when object editor is opened.
 * If rule set loses focus, the object's background turns from "selected" color
 * to white and user loses context (which object is shown in the object editor)
 */
    if (havePolicies)
    {
        RuleSetView* rv=dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget());

        if (rv && rv->getSelectedObject()!=getSelectedObject())
        {
            rv->unselect();
            rv->clearFocus();
        }
    }
    mainW->disableActions(havePolicies);
}

void ProjectPanel::editCopy()
{
    if (isManipulatorSelected()) copyObj();
    else
        if (m_panel->ruleSets->count()!=0)
            dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget())->copySelectedObject();
}

void ProjectPanel::editCut()
{
    if (isManipulatorSelected()) cutObj();
    else
        if (m_panel->ruleSets->count()!=0)
            dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget())->cutSelectedObject();
}

void ProjectPanel::editDelete()
{
    if (isManipulatorSelected()) deleteObj();
    //else
    //    if (ruleSets->count()!=0)
    //        dynamic_cast<RuleSetView*>(m_panel->ruleSets->currentWidget())->deleteSelectedObject();
}

void ProjectPanel::editPaste()
{
    if (isManipulatorSelected()) pasteObj();
    else
        if (m_panel->ruleSets->count()!=0)
            dynamic_cast<RuleSetView*>(
                m_panel->ruleSets->currentWidget())->pasteObject();
}

QString ProjectPanel::getDestDir(const QString &fname)
{
    QString destdir = "";

    if (st->getWDir().isEmpty())
    {
        if (fname.isEmpty())
        {
/* need some reasonable default working directory.
 * on Unix will use current dir.
 * on Windows will use user's document dir.
 */
#if defined(Q_OS_WIN32) || defined(Q_OS_MACX)
            destdir = userDataDir.c_str();
#else
            destdir = "";
#endif
        } else
        {
            if (QFileInfo(fname).isDir()) destdir=fname;
            else
                destdir = fname.left( fname.lastIndexOf('/',-1) );
        }
    } else
    {
        destdir=st->getWDir();
    }
    return destdir;
}

void ProjectPanel::setFileName(const QString &fname)
{
    systemFile = false;
    rcs->setFileName(fname);
    db()->setFileName(fname.toLatin1().constData());

    setWindowTitle(getPageTitle());
}

//wrapers for some ObjectManipulator functions
FWObject* ProjectPanel::getOpened()
{
    return m_panel->om->getOpened();
}

FWObject* ProjectPanel::getCurrentLib()
{
    return m_panel->om->getCurrentLib();
}

void ProjectPanel::updateObjectInTree(FWObject *obj, bool subtree)
{
    m_panel->om->updateObjectInTree(obj, subtree);
}

void ProjectPanel::loadDataFromFw(Firewall *fw)
{
    m_panel->om->loadObjects();

    if (fw)
    {
        m_panel->om->updateObjName(fw,"", false);
        m_panel->om->editObject(fw);
    }
}

void ProjectPanel::insertObjectInTree(FWObject *parent, FWObject *obj)
{
    m_panel->om->insertObjectInTree(parent, obj);
}

FWObject* ProjectPanel::createObject(const QString &objType,
                                     const QString &objName,
                                     FWObject *copyFrom)
{
    return m_panel->om->createObject(objType, objName, copyFrom);
}

FWObject* ProjectPanel::createObject(FWObject *parent,
                                     const QString &objType,
                                     const QString &objName,
                                     FWObject *copyFrom)
{
    return m_panel->om->createObject(parent, objType, objName, copyFrom);
}

void ProjectPanel::moveObject(FWObject *target,
                              FWObject *obj)
{
    m_panel->om->moveObject(target, obj);
}

void ProjectPanel::moveObject(const QString &targetLibName,
                              FWObject *obj)
{
    m_panel->om->moveObject(targetLibName, obj);
}

void ProjectPanel::updateLibColor(FWObject *lib)
{
    m_panel->om->updateLibColor(lib);
}

void ProjectPanel::updateLibName(FWObject *lib)
{
    m_panel->om->updateLibName(lib);
}

void ProjectPanel::updateObjName(FWObject *obj,
                                 const QString &oldName,
                                 bool  askForAutorename)
{
    m_panel->om->updateObjName(obj, oldName, askForAutorename);
}

void ProjectPanel::updateObjName(FWObject *obj,
                                 const QString &oldName,
                                 const QString &oldLabel,
                                 bool  askForAutorename)
{
    m_panel->om->updateObjName(obj, oldName, oldLabel, askForAutorename);
}


FWObject* ProjectPanel::pasteTo(FWObject *target, FWObject *obj)
{
    return m_panel->om->pasteTo(target, obj);
}

void ProjectPanel::delObj(FWObject *obj,bool openobj)
{
    m_panel->om->delObj(obj, openobj);
}

/*
 * Move object @obj from its current position in the tree to the @target
 * If successfull, returns pointer to @obj
 */
void ProjectPanel::relocateTo(FWObject *target, FWObject *obj)
{
    m_panel->om->relocateTo(target, obj);
}

ObjectTreeView* ProjectPanel::getCurrentObjectTree()
{
    return m_panel->om->getCurrentObjectTree();
}

void ProjectPanel::openObject(QTreeWidgetItem *otvi)
{
    m_panel->om->openObject(otvi);
}

void ProjectPanel::openObject(FWObject *obj)
{
    m_panel->om->openObject(obj);
}

bool ProjectPanel::editObject(FWObject *obj)
{
    return m_panel->om->editObject(obj);
}

void ProjectPanel::findAllFirewalls (std::list<Firewall *> &fws)
{
    m_panel->om->findAllFirewalls(fws);
}

FWObject* ProjectPanel::duplicateObject(FWObject *target,
                                        FWObject *obj,
                                        const QString &name,
                                        bool  askForAutorename)
{
    return m_panel->om->duplicateObject(target, obj, name, askForAutorename);
}

void ProjectPanel::showDeletedObjects(bool f)
{
    m_panel->om->showDeletedObjects(f);
}

void ProjectPanel::select()
{
    m_panel->om->select();
}

void ProjectPanel::unselect()
{
    m_panel->om->unselect();
}

void ProjectPanel::clearManipulatorFocus()
{
    m_panel->om->clearFocus();
}

void ProjectPanel::copyObj()
{
    m_panel->om->copyObj();
}

bool ProjectPanel::isManipulatorSelected()
{
    return m_panel->om->isSelected();
}

void ProjectPanel::cutObj()
{
    m_panel->om->cutObj();
}

void ProjectPanel::pasteObj()
{
    m_panel->om->pasteObj();
}


void ProjectPanel::newObject()
{
    m_panel->om->newObject();
}

void ProjectPanel::deleteObj()
{
    m_panel->om->deleteObj();
}

FWObject* ProjectPanel::getSelectedObject()
{
    return m_panel->om->getSelectedObject();
}

void ProjectPanel::reopenCurrentItemParent()
{
    m_panel->om->reopenCurrentItemParent();
}

void ProjectPanel::back()
{
    m_panel->om->back();
}

void ProjectPanel::lockObject()
{
    m_panel->om->lockObject();
}

void ProjectPanel::unlockObject()
{
    m_panel->om->unlockObject();
}



//wrapers for some Object Editor functions
bool ProjectPanel::isEditorVisible()
{
    return m_panel->bottomDockWidget->isVisible(); // editor
}

bool ProjectPanel::isEditorModified()
{
    return oe->isModified();
}

void ProjectPanel::showEditor()
{
    openEditorPanel();
    m_panel->objectEditorStack->setCurrentIndex(oe->getCurrentDialogIndex());
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_EDITOR_TAB);
    m_panel->bottomDockWidget->show(); // editor
}

void ProjectPanel::hideEditor()
{
    closeEditorPanel();
}

void ProjectPanel::closeEditor()
{
    m_panel->bottomDockWidget->close(); // editor
}

void ProjectPanel::openEditor(FWObject *o)
{
    QSize old_size = m_panel->objectEditorStack->size();
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_EDITOR_TAB);
    m_panel->bottomDockWidget->show(); // editor
    oe->open(o);
    m_panel->objectEditorStack->setCurrentIndex(oe->getCurrentDialogIndex());
    //m_panel->objectEditorFrame->show();
    m_panel->objectEditorStack->resize(old_size);
}

void ProjectPanel::openOptEditor(FWObject *o, ObjectEditor::OptType t)
{
    QSize old_size = m_panel->objectEditorStack->size();
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_EDITOR_TAB);
    m_panel->bottomDockWidget->show(); // editor
    oe->openOpt(o, t);
    m_panel->objectEditorStack->setCurrentIndex(oe->getCurrentDialogIndex());
    //m_panel->objectEditorFrame->show();
    m_panel->objectEditorStack->resize(old_size);
}

void ProjectPanel::blankEditor()
{
    oe->blank();
}


FWObject* ProjectPanel::getOpenedEditor()
{
    return oe->getOpened();
}

ObjectEditor::OptType ProjectPanel::getOpenedOptEditor()
{
    return oe->getOpenedOpt();
}

void ProjectPanel::selectObjectInEditor(FWObject *o)
{
    oe->selectObject(o);
}

void ProjectPanel::actionChangedEditor(FWObject *o)
{
    oe->actionChanged(o);
}

bool ProjectPanel::validateAndSaveEditor()
{
    return oe->validateAndSave();
}

void ProjectPanel::setFDObject(FWObject *o)
{
    fd->setObject(o);
    fd->show();
}
void ProjectPanel::resetFD()
{
    fd->reset();
}


void ProjectPanel::insertRule()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->insertRule();
}

void ProjectPanel::addRuleAfterCurrent()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->addRuleAfterCurrent();
}

void ProjectPanel::removeRule()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->removeRule();
}

void ProjectPanel::moveRule()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->moveRule();
}

void ProjectPanel::moveRuleUp()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->moveRuleUp();
}

void ProjectPanel::moveRuleDown()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->moveRuleDown();
}

void ProjectPanel::copyRule()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->copyRule();
}

void ProjectPanel::cutRule()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->cutRule();
}

void ProjectPanel::pasteRuleAbove()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->pasteRuleAbove();
}

void ProjectPanel::pasteRuleBelow()
{
    if (visibleRuleSet==NULL || m_panel->ruleSets->count()==0) return;
    getCurrentRuleSetView()->pasteRuleBelow();
}

bool ProjectPanel::editingLibrary()
{
    return (rcs!=NULL &&
        ( rcs->getFileName().endsWith(".fwl")) );
}

void ProjectPanel::createRCS(const QString &filename)
{
    rcs = new RCS(filename);
    systemFile = true;
}


QString ProjectPanel::getCurrentFileName()
{
    if (rcs!=NULL)  return rcs->getFileName();
    return "";
}

RCS * ProjectPanel::getRCS()
{
    return rcs;
}

void ProjectPanel::findObject(FWObject *o)
{
    findWhereUsedWidget->hide();
    if (fwbdebug) qDebug("ProjectPanel::findObject");
    findObjectWidget->findObject(o);
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_SEARCH_TAB); // search tab
    m_panel->bottomDockWidget->show();

}

void ProjectPanel::closeEditorPanel()
{
    //m_panel->objectEditorFrame->hide();
    m_panel->bottomDockWidget->hide(); // editor
}

void ProjectPanel::openEditorPanel()
{
    //m_panel->objectEditorFrame->show();

}

void ProjectPanel::search()
{
    findWhereUsedWidget->hide();
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_SEARCH_TAB); // search tab
    m_panel->bottomDockWidget->show();
    findObjectWidget->show();
}


void ProjectPanel::compile()
{
    if (isEditorVisible() &&
        !requestEditorOwnership(NULL,NULL,ObjectEditor::optNone,true))
        return;

    fileSave();
    mainW->compile();
}

void ProjectPanel::compile(set<Firewall*> vf)
{
    if (isEditorVisible() &&
        !requestEditorOwnership(NULL, NULL, ObjectEditor::optNone, true))
        return;

    fileSave();
    mainW->compile(vf);
}

void ProjectPanel::install(set<Firewall*> vf)
{
    mainW->install(vf);
}

void ProjectPanel::install()
{
    mainW->install();
}

void ProjectPanel::transferfw(set<Firewall*> vf)
{
    mainW->transferfw(vf);
}

void ProjectPanel::transferfw()
{
    mainW->transferfw();
}

void ProjectPanel::rollBackSelectionSameWidget()
{
    editorOwner->setFocus();
    emit restoreSelection_sign(true);
}

void ProjectPanel::rollBackSelectionDifferentWidget()
{
    editorOwner->setFocus();
    emit restoreSelection_sign(false);
}


QString ProjectPanel::printHeader()
{
    QString headerText = rcs->getFileName().section("/",-1,-1);
    if (rcs->isInRCS())
        headerText = headerText + ", rev " + rcs->getSelectedRev();
    return headerText;
}

void ProjectPanel::releaseEditor()
{
    disconnect( SIGNAL(restoreSelection_sign(bool)) );
}

void ProjectPanel::connectEditor(QWidget *w)
{
    connect(this, SIGNAL(restoreSelection_sign(bool)),
            w, SLOT(restoreSelection(bool)));
}

bool ProjectPanel::requestEditorOwnership(QWidget *w,
                                          FWObject *obj,
                                          ObjectEditor::OptType otype,
                                          bool validate)
{
    if (!isEditorVisible()) return false;

    if(obj == getOpenedEditor() &&
       otype == getOpenedOptEditor() &&
       w == editorOwner )
    {
        releaseEditor();
        editorOwner = w;
        connectEditor(editorOwner);
        return true;
    }

    if (validate && !validateAndSaveEditor())
    {
        /*
         * roll back selection in the widget that currently
         * owns the editor. Signal restoreSelection_sign
         * is still connected to the previous owner
         */
        if (w == editorOwner )
            QTimer::singleShot(0, this, SLOT(rollBackSelectionSameWidget()));
        else
            QTimer::singleShot(0,this,SLOT(rollBackSelectionDifferentWidget()));
        return false;
    }

    if (w)
    {
        releaseEditor();
        editorOwner = w;
        connectEditor(editorOwner);
    }
    return true;
}

bool ProjectPanel::validateForInsertion(FWObject *target, FWObject *obj)
{
    return objectTreeFormat->validateForInsertion(target, obj);
}

/*
 * TODO: move get*MenuState methods to ObjectManipulator
 */
bool ProjectPanel::getCopyMenuState(const QString &objPath)
{
    return objectTreeFormat->getCopyMenuState(objPath);
}

bool ProjectPanel::getCutMenuState(const QString &objPath)
{
    return objectTreeFormat->getCutMenuState(objPath);
}

bool ProjectPanel::getPasteMenuState(const QString &objPath)
{
    return objectTreeFormat->getPasteMenuState(objPath);
}

bool ProjectPanel::getDeleteMenuState(FWObject *obj)
{
    QString objPath = obj->getPath(true).c_str();
    bool del_menu_item_state = objectTreeFormat->getDeleteMenuState(objPath);

    // can't delete last policy, nat and routing child objects
    // also can't delete "top" policy ruleset
    if (del_menu_item_state && RuleSet::cast(obj))
    {
        if (dynamic_cast<RuleSet*>(obj)->isTop()) del_menu_item_state = false;
        else
        {
            FWObject *fw = obj->getParent();
            // fw can be NULL if this ruleset is in the Deleted objects
            // library
            if (fw==NULL) return del_menu_item_state;
            list<FWObject*> child_objects = fw->getByType(obj->getTypeName());
            if (child_objects.size()==1) del_menu_item_state = false;
        }
    }
    return del_menu_item_state;
}

FWObject* ProjectPanel::createNewLibrary(FWObjectDatabase *db)
{
    return objectTreeFormat->createNewLibrary(db);
}

void ProjectPanel::scheduleRuleSetRedraw()
{
    if (!ruleSetRedrawPending)
    {
        ruleSetRedrawPending = true;
        redrawRuleSets();
        //QTimer::singleShot( 0, this, SLOT(redrawRuleSets()) );
    }
}

void ProjectPanel::redrawRuleSets()
{
    ruleSetRedrawPending = false;
    reopenFirewall();
}

void ProjectPanel::findWhereUsed(FWObject * obj)
{
    findObjectWidget->hide();
    m_panel->bottomPanelTabWidget->setCurrentIndex(EDITOR_PANEL_SEARCH_TAB); // search tab
    m_panel->bottomDockWidget->show();
    findWhereUsedWidget->find(obj);
}

void ProjectPanel::showEvent(QShowEvent *ev)
{ 
    if (fwbdebug) qDebug("ProjectPanel::showEvent %p title=%s",
                         this, getPageTitle().toAscii().constData());
    QWidget::showEvent(ev);
}

void ProjectPanel::hideEvent(QHideEvent *ev)
{
    if (fwbdebug) qDebug("ProjectPanel::hideEvent %p title=%s",
                         this, getPageTitle().toAscii().constData());
    QWidget::hideEvent(ev);
}

void ProjectPanel::closeEvent(QCloseEvent * ev)
{   
    if (fwbdebug) qDebug("ProjectPanel::closeEvent %p title=%s",
                         this, getPageTitle().toAscii().constData());

    // Can't just call fileClose() because I need to ignore event in
    // case user clicks Cancel in dialog if some data has not been
    // saved.
    if (isEditorVisible())
    {
        if (!oe->validateAndSave())
        {
            ev->ignore();
            return;
        }
        closeEditorPanel();
    }

    if (!saveIfModified() || !checkin(true))
    {
        ev->ignore();
        return;
    }

    saveState();
    fileClose();

    mw->updateWindowTitle();

    QTimer::singleShot( 0, mw, SLOT(projectWindowClosed()) );

    if (fwbdebug) qDebug("ProjectPanel::closeEvent all done");
}

QString ProjectPanel::getFileName()
{
    if (rcs!=NULL)
    {
        QString FileName = rcs->getFileName();
        return FileName;
    }
    else
        return "";
}

void ProjectPanel::splitterMoved(int , int)
{
}

void ProjectPanel::resizeEvent(QResizeEvent*)
{
}

ProjectPanel * ProjectPanel::clone(ProjectPanel * cln)
{
    cln->mainW = mainW;
    cln->rcs = rcs;
    //cln->objectTreeFormat = objectTreeFormat;
    cln->systemFile = systemFile;
    cln->safeMode = safeMode;
    cln->editingStandardLib = editingStandardLib;
    cln->editingTemplateLib = editingTemplateLib;
    cln->ruleSetRedrawPending = ruleSetRedrawPending;
    //cln->objdb = objdb;
    cln->editorOwner = editorOwner;
    cln->oe = oe;
    cln->fd = fd;
    cln->autosaveTimer = autosaveTimer;
    //cln->ruleSetViews = ruleSetViews;
    cln->ruleSetTabIndex = ruleSetTabIndex;
    cln->visibleFirewall = visibleFirewall;
    cln->firewalls = firewalls;
    cln->lastFirewallIdx = lastFirewallIdx;
    cln->changingTabs = changingTabs;
    cln->noFirewalls = noFirewalls;
    cln->mdiWindow = mdiWindow ;
    cln->m_panel = m_panel;
    cln->findObjectWidget = findObjectWidget;
    cln->findWhereUsedWidget = findWhereUsedWidget;
    cln->copySet = copySet;
    return cln;
}

void ProjectPanel::updateLastModifiedTimestampForAllFirewalls(FWObject *obj)
{
    if (fwbdebug)
        qDebug("ProjectPanel::updateLastModifiedTimestampForAllFirewalls");

    if (obj==NULL) return;

    QStatusBar *sb = mw->statusBar();
    sb->showMessage( tr("Searching for firewalls affected by the change...") );

    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);

    QApplication::setOverrideCursor(QCursor( Qt::WaitCursor));

    list<Firewall *> fws = m_panel->om->findFirewallsForObject(obj);
    if (fws.size())
    {
        Firewall *f;
        for (list<Firewall *>::iterator i=fws.begin();
                i!=fws.end();
                ++i)
        {
            f = *i;
            if (f==obj) continue;

            f->updateLastModifiedTimestamp();
            QCoreApplication::postEvent(
                mw, new updateObjectInTreeEvent(getFileName(), f->getId()));

            list<Cluster*> clusters = m_panel->om->findClustersUsingFirewall(f);
            if (clusters.size() != 0)
            {
                list<Cluster*>::iterator it;
                for (it=clusters.begin(); it!=clusters.end(); ++it)
                {
                    Cluster *cl = *it;
                    cl->updateLastModifiedTimestamp();
                    QCoreApplication::postEvent(
                        mw, new updateObjectInTreeEvent(getFileName(), cl->getId()));
                }
            }
        }
    }
    QApplication::restoreOverrideCursor();
    sb->clearMessage();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,100);
}

void ProjectPanel::toggleViewTree(bool f)
{
    if (f) m_panel->treeDockWidget->show();
    else m_panel->treeDockWidget->hide();
}

void ProjectPanel::toggleViewRules(bool f)
{
    if (f) m_panel->rulesDockWidget->show();
    else m_panel->rulesDockWidget->hide();
}

void ProjectPanel::toggleViewEditor(bool f)
{
    if (f)
    {
        openEditor(m_panel->om->getOpened());
    } else m_panel->bottomDockWidget->hide(); // editor
}

/*
 * Signal QDockWidget::topLevelChanged is called after dock widget
 * is made floating or docked.
 */
void ProjectPanel::topLevelChangedForTreePanel(bool f)
{
    if (fwbdebug)
        qDebug() << "ProjectPanel::topLevelChangedForTreePanel  f=" << f;

    QList<int> sizes = m_panel->topSplitter->sizes();

#if defined(Q_WS_X11) 
    /*
     * QDockWidget object uses native decorators on Windows and Mac
     * and therefore gets window title bar there. On X11 QT emulates
     * title bar and allows dragging of the floating dock widget only
     * if its parent is QMainWindow. Here is a hack: we reparent the
     * widget in order to satisfy their requirements and make floating
     * panel widget draggable on all platforms. Need to reparent it
     * back and stick it into the layout of the ProjectPanel when it
     * is docked.
     */
    m_panel->treeDockWidget->disconnect(SIGNAL(topLevelChanged(bool)));
    m_panel->treeDockWidget->disconnect(SIGNAL(visibilityChanged(bool)));
    
    if (f)
    {
        m_panel->treeDockWidget->setParent(mw);
        mw->addDockWidget(Qt::LeftDockWidgetArea, m_panel->treeDockWidget);
        m_panel->treeDockWidget->show();
    } else
    {
        mw->removeDockWidget(m_panel->treeDockWidget);
        m_panel->treeDockWidget->setParent(m_panel->treeDockWidgetParentFrame);
        m_panel->treeDockWidgetParentFrameLayout->addWidget(m_panel->treeDockWidget, 0, 0, 1, 1);
        m_panel->treeDockWidget->show();
    }
    m_panel->treeDockWidget->setFloating(f);

    connect(m_panel->treeDockWidget, SIGNAL(topLevelChanged(bool)),
            this, SLOT(topLevelChangedForTreePanel(bool)));
    connect(m_panel->treeDockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(visibilityChangedForTreePanel(bool)));
#endif

    if (!m_panel->treeDockWidget->isWindow())
    {
        loadMainSplitter();
    } else
    {
        if (m_panel->treeDockWidget->isWindow())
        {
            // expand rules 
            collapseTree();
            m_panel->treeDockWidget->widget()->update();
        }
    }
}

void ProjectPanel::visibilityChangedForTreePanel(bool f)
{
    if (fwbdebug)
        qDebug() << "ProjectPanel::visibilityChangedForTreePanel  f="
                 << f
                 << " m_panel->treeDockWidget->isWindow()="
                 << m_panel->treeDockWidget->isWindow();

    QList<int> sizes = m_panel->topSplitter->sizes();

    if (f && !m_panel->treeDockWidget->isWindow())  // visible and not floating
    {
        loadMainSplitter();
    } else
    {
        // expand rules 
        collapseTree();
        m_panel->treeDockWidget->widget()->update();
    }
}

void ProjectPanel::topLevelChangedForBottomPanel(bool f)
{
#if defined(Q_WS_X11) 
    if (f)
    {
        m_panel->bottomDockWidget->setParent(mw);
        mw->addDockWidget(Qt::BottomDockWidgetArea, m_panel->bottomDockWidget);
        m_panel->bottomDockWidget->show();
    } else
    {
        mw->removeDockWidget(m_panel->bottomDockWidget);
        m_panel->bottomDockWidget->setParent(this);
        layout()->addWidget(m_panel->bottomDockWidget);
        m_panel->bottomDockWidget->show();
    }
    m_panel->bottomDockWidget->setFloating(f);
#endif
}

