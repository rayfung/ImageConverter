#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QStringListModel>
#include <QCloseEvent>

namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_pushButtonConvert_clicked();
    void on_pushButtonAddImage_clicked();
    void on_pushButtonRemoveImage_clicked();
    void on_pushButtonChooseTarget_clicked();
    void onSelectionChanged();

private:
    void batchConvert(const QStringList &pathList, const QString &format, bool removeAlpha);
    void loadSettings();
    void saveSettings();
    void showPreview(const QString &path);

private:
    Ui::MainWidget *ui;
    QStringListModel *model;
    QString lastOpenPath;
    QString lastSavePath;
};

#endif // MAINWIDGET_H
