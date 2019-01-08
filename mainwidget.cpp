#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QFileDialog>
#include <QStringList>
#include <QMessageBox>
#include <QImageReader>
#include <QImageWriter>
#include <QSettings>
#include <QSplitter>

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    model(new QStringListModel())
{
    ui->setupUi(this);

    ui->comboBoxFormat->addItem("PNG");
    ui->comboBoxFormat->addItem("JPG");

    ui->listViewSource->setModel(model);

    loadSettings();

    connect(ui->listViewSource->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onSelectionChanged()));
}

MainWidget::~MainWidget()
{
    delete model;
    delete ui;
}

void MainWidget::loadSettings()
{
    QSettings settings("Ray Fung", "Image Converter");

    lastOpenPath = settings.value("last_open_path").toString();
    lastSavePath = settings.value("last_save_path").toString();
}

void MainWidget::saveSettings()
{
    QSettings settings("Ray Fung", "Image Converter");

    settings.setValue("last_open_path", lastOpenPath);
    settings.setValue("last_save_path", lastSavePath);
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWidget::on_pushButtonConvert_clicked()
{
    int answer = QMessageBox::question(this, QString("批量转换"), QString("确定要开始批量转换吗？"));

    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QStringList pathList = model->stringList();
    QString format = ui->comboBoxFormat->currentText();
    bool keepAlpha = ui->checkBoxAlpha->isChecked();

    batchConvert(pathList, format, !keepAlpha);
}

static void maybeRemoveAlpha(QImage *image)
{
    if (image->hasAlphaChannel()) {
        switch (image->format()) {
        case QImage::Format_RGBA8888:
        case QImage::Format_RGBA8888_Premultiplied:
            *image = image->convertToFormat(QImage::Format_RGBX8888);
            break;
        case QImage::Format_A2BGR30_Premultiplied:
            *image = image->convertToFormat(QImage::Format_BGR30);
            break;
        case QImage::Format_A2RGB30_Premultiplied:
            *image = image->convertToFormat(QImage::Format_RGB30);
            break;
        default:
            *image = image->convertToFormat(QImage::Format_RGB32);
            break;
        }
    }
}

void MainWidget::batchConvert(const QStringList &pathList, const QString &format, bool removeAlpha)
{
    QString savedTitle = this->windowTitle();
    QString targetPath = ui->lineEditTarget->text();
    QFileInfo targetInfo(targetPath);

    if (targetInfo.isDir() == false)
    {
        QMessageBox::information(this, QString("提示"), QString("请设置正确的保存目录"));
        return;
    }

    this->setEnabled(false);
    qApp->processEvents();

    //保存成 JPG 格式时不需要手动去除 alpha 通道
    if (format.toLower() == "jpg")
    {
        removeAlpha = false;
    }

    for (int i = 0; i < pathList.size(); ++i)
    {
        QString path = pathList.at(i);
        QFileInfo info(path);
        QString targetFile = targetPath + "/" + info.completeBaseName() + "." + format.toLower();

        this->setWindowTitle(QString("正在转换 %1").arg(path));
        qApp->processEvents();

        QImageReader reader(path);
        QImage image = reader.read();

        if (image.isNull())
        {
            QMessageBox::critical(this, QString("转换出错"), QString("%1\n\n%2").arg(path, reader.errorString()));
            break;
        }

        if (removeAlpha)
        {
            maybeRemoveAlpha(&image);
        }

        bool ok = image.save(targetFile);
        if (!ok)
        {
            QMessageBox::critical(this, QString("转换出错"), QString("%1\n\n保存失败").arg(targetFile));
            break;
        }
    }

    this->setEnabled(true);
    this->setWindowTitle(savedTitle);
}

void MainWidget::on_pushButtonAddImage_clicked()
{
    QStringList fileList = QFileDialog::getOpenFileNames(this, QString("加入图像"), lastOpenPath, QString("Images (*.png *.jpg *.bmp *.jpeg)"));

    if (fileList.isEmpty())
    {
        return;
    }

    QFileInfo firstInfo(fileList.at(0));
    lastOpenPath = firstInfo.absolutePath();

    fileList = model->stringList() + fileList;
    fileList.removeDuplicates();
    model->setStringList(fileList);
}

void MainWidget::on_pushButtonRemoveImage_clicked()
{
    QModelIndexList selectedList = ui->listViewSource->selectionModel()->selectedIndexes();

    if (selectedList.isEmpty())
    {
        return;
    }

    QStringList strList = model->stringList();

    for (int i = 0; i < selectedList.size(); ++i)
    {
        QString s = model->data(selectedList.at(i), Qt::DisplayRole).toString();
        strList.removeAll(s);
    }

    model->setStringList(strList);
}

void MainWidget::on_pushButtonChooseTarget_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, QString("选择输出目录"), lastSavePath);

    if (path.isEmpty())
    {
        return;
    }

    lastSavePath = path;
    ui->lineEditTarget->setText(path);
}

void MainWidget::onSelectionChanged()
{
    QModelIndexList selectedList = ui->listViewSource->selectionModel()->selectedIndexes();

    if (selectedList.isEmpty())
    {
        ui->labelPreview->clear();
        ui->labelInfo->clear();
    }
    else if (selectedList.size() == 1)
    {
        QString path = model->data(selectedList.at(0), Qt::DisplayRole).toString();
        showPreview(path);
    }
    else
    {
        ui->labelPreview->setText("选中了多张图像");
        ui->labelInfo->clear();
    }
}

void MainWidget::showPreview(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);

    QSize rawSize = reader.size();
    QSize actualSize;
    if (reader.transformation() & QImageIOHandler::TransformationRotate90)
    {
        actualSize = rawSize.transposed();
    }
    else
    {
        actualSize = rawSize;
    }

    const int MAX_PIXEL = 256;

    if (rawSize.width() > MAX_PIXEL || rawSize.height() > MAX_PIXEL)
    {
        double factor;

        if (rawSize.width() > rawSize.height())
        {
            factor = (double)MAX_PIXEL / (double)rawSize.width();
        }
        else
        {
            factor = (double)MAX_PIXEL / (double)rawSize.height();
        }

        QSize scaledSize = rawSize * factor;

        if (scaledSize.width() <= 0)
        {
            scaledSize.setWidth(1);
        }

        if (scaledSize.height() <= 0)
        {
            scaledSize.setHeight(1);
        }

        reader.setScaledSize(scaledSize);
    }

    QPixmap pixmap = QPixmap::fromImageReader(&reader);
    if (pixmap.isNull())
    {
        ui->labelPreview->setText("无法打开图像");
        ui->labelInfo->clear();
    }
    else
    {
        ui->labelPreview->setPixmap(pixmap);
        ui->labelInfo->setText(QString("%1x%2").arg(actualSize.width()).arg(actualSize.height()));
    }
}
