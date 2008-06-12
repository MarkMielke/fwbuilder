
#if defined(Q_OS_WIN32)
#  include <stdio.h>
#  include <sys/timeb.h>
#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <pwd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>

#if defined __cplusplus

#include "../../config.h"
#include "VERSION.h"
#include "build_num"
#include "definitions.h"
#include "global.h"
#include "utils.h"

#include "fwbuilder/Address.h"
#include "fwbuilder/AddressRange.h"
#include "fwbuilder/AddressTable.h"
#include "fwbuilder/BackgroundOp.h"
#include "fwbuilder/Constants.h"
#include "fwbuilder/CustomService.h"
#include "fwbuilder/DNSName.h"
#include "fwbuilder/FWException.h"
#include "fwbuilder/FWObject.h"
#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/FWOptions.h"
#include "fwbuilder/FWReference.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Group.h"
#include "fwbuilder/Host.h"
#include "fwbuilder/ICMPService.h"
#include "fwbuilder/IPService.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/InetAddr.h"
#include "fwbuilder/InetAddrMask.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/InterfacePolicy.h"
#include "fwbuilder/Interval.h"
#include "fwbuilder/IntervalGroup.h"
#include "fwbuilder/Library.h"
#include "fwbuilder/Logger.h"
#include "fwbuilder/Management.h"
#include "fwbuilder/NAT.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/ObjectGroup.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/Routing.h"
#include "fwbuilder/Rule.h"
#include "fwbuilder/RuleElement.h"
#include "fwbuilder/RuleSet.h"
#include "fwbuilder/ServiceGroup.h"
#include "fwbuilder/TCPService.h"
#include "fwbuilder/TagService.h"
#include "fwbuilder/Tools.h"
#include "fwbuilder/UDPService.h"
#include "fwbuilder/XMLTools.h"
#include "fwbuilder/libfwbuilder-config.h"
#include "fwbuilder/physAddress.h"

// QT

#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFocusEvent>
#include <QHeaderView>
#include <QHideEvent>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>
#include <qaction.h>
#include <qapplication.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcolordialog.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qdialog.h>
#include <qdir.h>
#include <qdrag.h>
#include <qevent.h>
#include <qeventloop.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qframe.h>
#include <qglobal.h>
#include <qgroupbox.h>
#include <qheaderview.h>
#include <qhostaddress.h>
#include <qhostinfo.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qlistwidget.h>
#include <qmap.h>
#include <qmenu.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qprintdialog.h>
#include <qprinter.h>
#include <qprocess.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qrect.h>
#include <qregexp.h>
#include <qsettings.h>
#include <qspinbox.h>
#include <qsplitter.h>
#include <qstackedwidget.h>
#include <qstatusbar.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qstyle.h>
#include <qtableview.h>
#include <qtablewidget.h>
#include <qtabwidget.h>
#include <qtextbrowser.h>
#include <qtextcodec.h>
#include <qtextedit.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qtreewidget.h>
#include <qvariant.h>
#include <qvector.h>
#include <qwidget.h>

// STL

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#endif
