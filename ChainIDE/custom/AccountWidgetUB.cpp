#include "AccountWidgetUB.h"
#include "ui_AccountWidgetUB.h"

#include <QMenu>
#include <QClipboard>
#include "ChainIDE.h"
#include "datamanager/DataManagerUB.h"
#include "NameDialogUB.h"
#include "ImportDialogUB.h"
#include "IDEUtil.h"

class AccountWidgetUB::DataPrivate
{
public:
    DataPrivate()
        :contextMenu(new QMenu())
    {

    }
public:
    QMenu *contextMenu;//右键菜单
    QList<QTreeWidgetItem*> findItems;
};

AccountWidgetUB::AccountWidgetUB(QWidget *parent) :
    MoveableDialog(parent),
    ui(new Ui::AccountWidgetUB),
    _p(new DataPrivate())
{
    ui->setupUi(this);
    InitWidget();
}

AccountWidgetUB::~AccountWidgetUB()
{
    delete _p;
    delete ui;
}

void AccountWidgetUB::jsonDataUpdated(const QString &id, const QString &data)
{
    if("createnewaccount" == id)
    {
        DataManagerUB::getInstance()->queryAccount();
    }
}

void AccountWidgetUB::on_newAccount_clicked()
{
    NameDialogUB dia;
    QString name = dia.pop();
    if(!name.isEmpty())
    {
        ChainIDE::getInstance()->postRPC("createnewaccount",IDEUtil::toJsonFormat("getnewaddress",QJsonArray()<<name));
    }
}

void AccountWidgetUB::on_importKey_clicked()
{
    ImportDialogUB dia;
    dia.exec();
}

void AccountWidgetUB::on_closeBtn_clicked()
{
    close();
}

void AccountWidgetUB::CopyAddr()
{
    if(QTreeWidgetItem *item = ui->treeWidget->currentItem())
    {
        QApplication::clipboard()->setText(item->text(0));
    }
}

bool AccountWidgetUB::eventFilter(QObject *o, QEvent *e)
{
    if(o == ui->treeWidget && e->type() == QEvent::ContextMenu)
    {
        if(QTreeWidgetItem *item = ui->treeWidget->currentItem())
        {
            if(ui->treeWidget->currentColumn() == 0 && ui->treeWidget->itemAt(ui->treeWidget->viewport()->mapFromGlobal(QCursor::pos())) == item)
            {
                _p->contextMenu->exec(QCursor::pos());
            }
        }
    }
    return QWidget::eventFilter(o,e);
}

void AccountWidgetUB::InitWidget()
{
    //
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    //初始化右键菜单
    InitContextMenu();

    ui->treeWidget->installEventFilter(this);

    //查找所有账户信息
    connect(ChainIDE::getInstance(),&ChainIDE::jsonDataUpdated,this,&AccountWidgetUB::jsonDataUpdated);
    connect(ui->lineEdit,&QLineEdit::textEdited,this, &AccountWidgetUB::QuerySlots);
    connect(ui->lineEdit,&QLineEdit::returnPressed,this,&AccountWidgetUB::QuerySlots);

    connect(DataManagerUB::getInstance(),&DataManagerUB::queryAccountFinish,this,&AccountWidgetUB::InitTree);
    DataManagerUB::getInstance()->queryAccount();
}

void AccountWidgetUB::InitTree()
{
    ui->treeWidget->clear();
    DataManagerStruct::AccountUB::AccountDataPtr data = DataManagerUB::getInstance()->getAccount();
    ui->treeWidget->setColumnCount(3);
    for(auto it = data->getAccount().begin();it != data->getAccount().end();++it)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList()<<(*it)->getAccountName()<<QString::number((*it)->getTotalBalance(),'f',8)<<"");
        item->setTextAlignment(0,Qt::AlignCenter);
        item->setTextAlignment(1,Qt::AlignCenter);
        item->setTextAlignment(2,Qt::AlignCenter);
        ui->treeWidget->addTopLevelItem(item);

        for(auto add = (*it)->getAddressInfos().begin();add != (*it)->getAddressInfos().end();++add)
        {
            QTreeWidgetItem *childitem = new QTreeWidgetItem(QStringList()<<(*add)->GetAddress()<<QString::number((*add)->GetBalance(),'f',8)<<tr("转账"));
            childitem->setTextAlignment(0,Qt::AlignCenter);
            childitem->setTextAlignment(1,Qt::AlignCenter);
            childitem->setTextAlignment(2,Qt::AlignCenter);
            item->addChild(childitem);
        }
    }
    ui->treeWidget->setHeaderLabels(QStringList()<<tr("账户/地址")<<tr("余额")<<tr("操作"));
    ui->treeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
    ui->treeWidget->expandAll();
    ui->treeWidget->hideColumn(2);
}

void AccountWidgetUB::InitContextMenu()
{
    QAction *copyAction = new QAction(tr("复制"),this);
    connect(copyAction,&QAction::triggered,this,&AccountWidgetUB::CopyAddr);
    _p->contextMenu->addAction(copyAction);
}

void AccountWidgetUB::QuerySlots()
{
    QString queryString = ui->lineEdit->text();
    if(queryString.isEmpty())
    {
        ui->treeWidget->clearSelection();
        _p->findItems.clear();
        return;
    }

    QList<QTreeWidgetItem*> findItems;
    QTreeWidgetItemIterator it(ui->treeWidget);
    while(*it)
    {
        if(-1 != (*it)->text(0).indexOf(queryString,0,Qt::CaseInsensitive))
        {
            findItems.push_back(*it);
        }
        ++it;
    }
    if(findItems.empty())
    {
        ui->treeWidget->clearSelection();
        _p->findItems.clear();
        return;
    }

    if(!isEqualList(findItems,_p->findItems))
    {//说明不是上次搜索的结果，取第一个
        ui->treeWidget->clearSelection();
        _p->findItems = findItems;
        ui->treeWidget->setCurrentItem(_p->findItems.front());
    }
    else
    {//说明是上次搜索的结果
        selectNext(_p->findItems);
    }
}

void AccountWidgetUB::selectNext(const QList<QTreeWidgetItem *> &listItem)
{
    if(listItem.empty()) return;
    int index = listItem.indexOf(ui->treeWidget->currentItem());
    if(-1 == index || listItem.count() - 1 == index)
    {
        ui->treeWidget->clearSelection();
        ui->treeWidget->setCurrentItem(listItem.front());
    }
    else if(listItem.at(index + 1))
    {
        ui->treeWidget->clearSelection();
        ui->treeWidget->setCurrentItem(listItem.at(index+1));
    }
}

bool AccountWidgetUB::isEqualList(const QList<QTreeWidgetItem *> &list1, const QList<QTreeWidgetItem *> &list2)
{
    if(list1.count() == list2.count())
    {
        foreach (QTreeWidgetItem *item, list1) {
            if(!list2.contains(item))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}
