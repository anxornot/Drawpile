/*
   Drawpile - a collaborative drawing program.

   Copyright (C) 2014-2017 Calle Laakkonen

   Drawpile is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Drawpile is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Drawpile.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "loginsessions.h"
#include "utils/icon.h"

#include <QDebug>
#include <QPixmap>

namespace net {

LoginSessionModel::LoginSessionModel(QObject *parent) :
	QAbstractTableModel(parent)
{
}

int LoginSessionModel::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid())
		return 0;
	return m_sessions.size();
}

int LoginSessionModel::columnCount(const QModelIndex &parent) const
{
	if(parent.isValid())
		return 0;

	// Columns:
	// 0 - closed/incompatible/password needed status icon
	// 1 - title
	// 2 - session founder name
	// 3 - user count

	return 4;
}

QVariant LoginSessionModel::data(const QModelIndex &index, int role) const
{

	const LoginSession &ls = m_sessions.at(index.row());

	if(role == Qt::DisplayRole) {
		switch(index.column()) {
		case 1: {
			QString title = ls.title.isEmpty() ? tr("(untitled)") : ls.title;
			if(!ls.alias.isEmpty())
				title = QStringLiteral("%1 [%2]").arg(title).arg(ls.alias);
			return title;
		}
		case 2: return ls.founder;
		case 3: return ls.userCount;
		}

	} else if(role == Qt::DecorationRole) {
		if(index.column()==0) {
			if(ls.closed)
				return icon::fromTheme("im-ban-user").pixmap(16, 16);
			else if(ls.needPassword)
				return icon::fromTheme("object-locked").pixmap(16, 16);
		}

	} else if(role == Qt::ToolTipRole) {
		if(ls.incompatible)
			return tr("Incompatible version");

	} else {
		switch(role) {
		case IdRole: return ls.id;
		case IdAliasRole: return ls.alias;
		case UserCountRole: return ls.userCount;
		case TitleRole: return ls.title;
		case FounderRole: return ls.founder;
		case NeedPasswordRole: return ls.needPassword;
		case PersistentRole: return ls.persistent;
		case ClosedRole: return ls.closed;
		case IncompatibleRole: return ls.incompatible;
		case JoinableRole: return !(ls.closed | ls.incompatible);
		}
	}

	return QVariant();
}

Qt::ItemFlags LoginSessionModel::flags(const QModelIndex &index) const
{
	const LoginSession &ls = m_sessions.at(index.row());
	if(ls.incompatible || ls.closed)
		return Qt::NoItemFlags;
	else
		return QAbstractTableModel::flags(index);
}

QVariant LoginSessionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	switch(section) {
	case 1: return tr("Title");
	case 2: return tr("Started by");
	case 3: return tr("Users");
	}

	return QVariant();
}

void LoginSessionModel::updateSession(const LoginSession &session)
{
	int oldIndex=-1;
	for(int i=0;i<m_sessions.size();++i) {
		if(m_sessions.at(i).id == session.id) {
			oldIndex = i;
			break;
		}
	}

	if(oldIndex>=0) {
		m_sessions[oldIndex] = session;
		emit dataChanged(index(oldIndex, 0), index(oldIndex, columnCount()));
	} else {
		beginInsertRows(QModelIndex(), m_sessions.size(), m_sessions.size());
		m_sessions.append(session);
		endInsertRows();
	}
}

void LoginSessionModel::removeSession(const QString &id)
{
	int idx=-1;
	for(int i=0;i<m_sessions.size();++i) {
		if(m_sessions.at(i).id == id) {
			idx = i;
			break;
		}
	}
	if(idx<0) {
		qWarning() << "removeSession: id" << id << "does not exist!";
	} else {
		beginRemoveRows(QModelIndex(), idx, idx);
		m_sessions.removeAt(idx);
		endRemoveRows();
	}
}

}
