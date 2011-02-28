#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTabWidget>
#include <QMainWindow>

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

public slots:
    void getCollectionPath();

private slots:
    void showAboutDialog();

private:
    QTabWidget m_tabWidget;
};

#endif // MAINWINDOW_H
