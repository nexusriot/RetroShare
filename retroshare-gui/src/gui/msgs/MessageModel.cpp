/*******************************************************************************
 * retroshare-gui/src/gui/msgs/RsMessageModel.cpp                              *
 *                                                                             *
 * Copyright 2019 by Cyril Soler <csoler@users.sourceforge.net>                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <list>

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QModelIndex>
#include <QIcon>

#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/gxs/GxsIdDetails.h"
#include "MessageModel.h"
#include "retroshare/rsexpr.h"

//#define DEBUG_MESSAGE_MODEL

#define IS_MESSAGE_UNREAD(flags) (flags & RS_MSG_UNREAD_BY_USER)

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsMessageModel::FilterString("filtered");

RsMessageModel::RsMessageModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    mFilteringEnabled=false;
}

void RsMessageModel::preMods()
{
 	emit layoutAboutToBeChanged();
}
void RsMessageModel::postMods()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(0,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
}

// void RsGxsForumModel::setSortMode(SortMode mode)
// {
//     preMods();
//
//     mSortMode = mode;
//
//     postMods();
// }

int RsMessageModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
        return 0;

    if(parent.column() > 0)
        return 0;

    if(mMessages.empty())	// security. Should never happen.
        return 0;

	if(parent.internalPointer() == NULL)
		return mMessages.size();

    return 0;
}

int RsMessageModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

bool RsMessageModel::getMessageData(const QModelIndex& i,Rs::Msgs::MessageInfo& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t index = 0;

	if(!convertInternalIdToMsgIndex(ref,index) || index >= mMessages.size())
		return false ;

	return rsMsgs->getMessage(mMessages[index].msgId,fmpe);
}

bool RsMessageModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

    return false ;
}

bool RsMessageModel::convertMsgIndexToInternalId(uint32_t index,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    id = index + 1 ;

	return true;
}

bool RsMessageModel::convertInternalIdToMsgIndex(quintptr ref,uint32_t& index)
{
    if(ref == 0)
        return false ;

    index = ref - 1;
	return true;
}

QModelIndex RsMessageModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_THREAD_NB_COLUMNS)
		return QModelIndex();

	quintptr ref ;

    if(parent.internalId() == 0 && convertMsgIndexToInternalId(row,ref))
		return createIndex(row,column,ref) ;

    return QModelIndex();
}

QModelIndex RsMessageModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

	return QModelIndex();
}

Qt::ItemFlags RsMessageModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant RsMessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
		switch(section)
		{
		case COLUMN_THREAD_DATE:         return tr("Date");
		case COLUMN_THREAD_AUTHOR:       return tr("Author");
		default:
			return QVariant();
		}

	if(role == Qt::DecorationRole)
		switch(section)
		{
		case COLUMN_THREAD_READ:         return QIcon(":/images/message-state-read.png");
		default:
			return QVariant();
		}

	return QVariant();
}

QVariant RsMessageModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_FORUMMODEL
    std::cerr << "calling data(" << index << ") role=" << role << std::endl;
#endif

	if(!index.isValid())
		return QVariant();

	switch(role)
	{
	case Qt::SizeHintRole: return sizeHintRole(index.column()) ;
    case Qt::StatusTipRole:return QVariant();
    default: break;
	}

	quintptr ref = (index.isValid())?index.internalId():0 ;
	uint32_t entry = 0;

#ifdef DEBUG_FORUMMODEL
	std::cerr << "data(" << index << ")" ;
#endif

	if(!ref)
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << " [empty]" << std::endl;
#endif
		return QVariant() ;
	}

	if(!convertInternalIdToMsgIndex(ref,entry) || entry >= mMessages.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const Rs::Msgs::MsgInfoSummary& fmpe(mMessages[entry]);

    if(role == Qt::FontRole)
    {
        QFont font ;
//		font.setBold( (fmpe.mPostFlags & (ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN | ForumModelPostEntry::FLAG_POST_IS_PINNED)) || IS_MSG_UNREAD(fmpe.mMsgStatus));
        return QVariant(font);
    }

#ifdef DEBUG_FORUMMODEL
	std::cerr << " [ok]" << std::endl;
#endif

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;

	case FilterRole:         return filterRole    (fmpe,index.column()) ;
	case StatusRole:         return statusRole    (fmpe,index.column()) ;
	case SortRole:           return sortRole      (fmpe,index.column()) ;
	default:
		return QVariant();
	}
}

QVariant RsMessageModel::textColorRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
//    if( (fmpe.msgflags & ForumModelPostEntry::FLAG_POST_IS_MISSING))
//        return QVariant(mTextColorMissing);
//
//    if(IS_MSG_UNREAD(fmpe.mMsgStatus) || (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED))
//        return QVariant(mTextColorUnread);
//    else
//        return QVariant(mTextColorRead);

	return QVariant();
}

QVariant RsMessageModel::statusRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
// 	if(column != COLUMN_THREAD_DATA)
//        return QVariant();

    return QVariant();//fmpe.mMsgStatus);
}

QVariant RsMessageModel::filterRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    if(!mFilteringEnabled)
        return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsMessageModel::updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings)
{
    QString s ;
	uint32_t count = 0;

	return count;
}


void RsMessageModel::setFilter(int column,const QStringList& strings,uint32_t& count)
{
    preMods();

    if(!strings.empty())
    {
		count = updateFilterStatus(ForumModelIndex(0),column,strings);
        mFilteringEnabled = true;
    }
    else
    {
		count=0;
        mFilteringEnabled = false;
    }

	postMods();
}

QVariant RsMessageModel::toolTipRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    if(column == COLUMN_THREAD_AUTHOR)
	{
		QString str,comment ;
		QList<QIcon> icons;

		if(!GxsIdDetails::MakeIdDesc(RsGxsId(fmpe.srcId.toStdString()), true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
			return QVariant();

		int S = QFontMetricsF(QApplication::font()).height();
		QImage pix( (*icons.begin()).pixmap(QSize(4*S,4*S)).toImage());

		QString embeddedImage;
		if(RsHtml::makeEmbeddedImage(pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, 8*S * 8*S))
			comment = "<table><tr><td>" + embeddedImage + "</td><td>" + comment + "</td></table>";

		return comment;
	}

    return QVariant();
}

QVariant RsMessageModel::backgroundRole(const Rs::Msgs::MsgInfoSummary &fmpe, int column) const
{
    return QVariant();
}

QVariant RsMessageModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	switch(col)
	{
	default:
	case COLUMN_THREAD_SUBJECT:      return QVariant( QSize(factor * 170, factor*14 ));
	case COLUMN_THREAD_DATE:         return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_AUTHOR:       return QVariant( QSize(factor * 75 , factor*14 ));
	}
}

QVariant RsMessageModel::authorRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
//    if(column == COLUMN_THREAD_DATA)
//        return QVariant(QString::fromStdString(fmpe.mAuthorId.toStdString()));

    return QVariant();
}

QVariant RsMessageModel::sortRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    switch(column)
    {
	case COLUMN_THREAD_DATE:         return QVariant(QString::number(fmpe.ts)); // we should probably have leading zeroes here

	case COLUMN_THREAD_READ:         return QVariant((bool)IS_MESSAGE_UNREAD(fmpe.msgflags));
    case COLUMN_THREAD_AUTHOR:
    {
        QString str,comment ;
        QList<QIcon> icons;
		GxsIdDetails::MakeIdDesc(RsGxsId(fmpe.srcId), false, str, icons, comment,GxsIdDetails::ICON_TYPE_NONE);

        return QVariant(str);
    }
    default:
        return displayRole(fmpe,column);
    }
}

QVariant RsMessageModel::displayRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
	switch(col)
	{
		case COLUMN_THREAD_SUBJECT:   return QVariant(QString::fromUtf8(fmpe.title.c_str()));

		case COLUMN_THREAD_READ:return QVariant();
    	case COLUMN_THREAD_DATE:{
    							    QDateTime qtime;
									qtime.setTime_t(fmpe.ts);

									return QVariant(DateTime::formatDateTime(qtime));
    							}

		case COLUMN_THREAD_AUTHOR: return QVariant();

		default:
			return QVariant("[ TODO ]");
		}


	return QVariant("[ERROR]");
}

QVariant RsMessageModel::userRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
	switch(col)
    {
     	case COLUMN_THREAD_AUTHOR:   return QVariant(QString::fromStdString(fmpe.srcId.toStdString()));
     	case COLUMN_THREAD_MSGID:    return QVariant(QString::fromStdString(fmpe.msgId));
    default:
        return QVariant();
    }
}

QVariant RsMessageModel::decorationRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
    if(col == COLUMN_THREAD_READ)
        return QVariant(IS_MESSAGE_UNREAD(fmpe.msgflags));
    else
		return QVariant();
}

void RsMessageModel::clear()
{
    preMods();

    mMessages.clear();

	postMods();

	emit messagesLoaded();
}

void RsMessageModel::setMessages(const std::list<Rs::Msgs::MsgInfoSummary>& msgs)
{
    preMods();

    beginRemoveRows(QModelIndex(),0,mMessages.size()-1);
    endRemoveRows();

    mMessages.clear();

    for(auto it(msgs.begin());it!=msgs.end();++it)
    	mMessages.push_back(*it);

    // now update prow for all posts

#ifdef DEBUG_FORUMMODEL
    debug_dump();
#endif

    beginInsertRows(QModelIndex(),0,mMessages.size()-1);
    endInsertRows();
	postMods();

	emit messagesLoaded();
}

void RsMessageModel::updateMessages()
{
    std::list<Rs::Msgs::MsgInfoSummary> msgs ;

    rsMsgs->getMessageSummaries(msgs);
	setMessages(msgs);
}

//	RsThread::async([this, group_id]()
//	{
//        // 1 - get message data from p3GxsForums
//
//        std::list<RsGxsGroupId> forumIds;
//		std::vector<RsMsgMetaData> msg_metas;
//		std::vector<RsGxsForumGroup> groups;
//
//        forumIds.push_back(group_id);
//
//		if(!rsGxsForums->getForumsInfo(forumIds,groups))
//		{
//			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum group info for forum " << group_id << std::endl;
//			return;
//        }
//
//		if(!rsGxsForums->getForumMsgMetaData(group_id,msg_metas))
//		{
//			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum message info for forum " << group_id << std::endl;
//			return;
//		}
//
//        // 2 - sort the messages into a proper hierarchy
//
//        auto post_versions = new std::map<RsGxsMessageId,std::vector<std::pair<time_t, RsGxsMessageId> > >() ;
//        std::vector<ForumModelPostEntry> *vect = new std::vector<ForumModelPostEntry>();
//        RsGxsForumGroup group = groups[0];
//
//        computeMessagesHierarchy(group,msg_metas,*vect,*post_versions);
//
//        // 3 - update the model in the UI thread.
//
//        RsQThreadUtils::postToObject( [group,vect,post_versions,this]()
//		{
//			/* Here it goes any code you want to be executed on the Qt Gui
//			 * thread, for example to update the data model with new information
//			 * after a blocking call to RetroShare API complete, note that
//			 * Qt::QueuedConnection is important!
//			 */
//
//            setPosts(group,*vect,*post_versions) ;
//
//            delete vect;
//            delete post_versions;
//
//
//		}, this );
//
//    });

static bool decreasing_time_comp(const std::pair<time_t,RsGxsMessageId>& e1,const std::pair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void RsMessageModel::setMsgReadStatus(const QModelIndex& i,bool read_status)
{
	if(!i.isValid())
		return ;

    preMods();

	quintptr ref = i.internalId();
	uint32_t index = 0;

	if(!convertInternalIdToMsgIndex(ref,index) || index >= mMessages.size())
		return ;

    rsMsgs->MessageRead(mMessages[index].msgId,!read_status);

    postMods();
}

QModelIndex RsMessageModel::getIndexOfMessage(const std::string& mid) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    for(uint32_t i=0;i<mMessages.size();++i)
        if(mMessages[i].msgId == mid)
        {
            quintptr ref ;
            convertMsgIndexToInternalId(i,ref);

            return createIndex(i,0,ref);
        }

    return QModelIndex();
}

void RsMessageModel::debug_dump() const
{
    for(auto it(mMessages.begin());it!=mMessages.end();++it)
		std::cerr << "Id: " << it->msgId << ": from " << it->srcId << ": flags=" << it->msgflags << ": title=\"" << it->title << "\"" << std::endl;
}

