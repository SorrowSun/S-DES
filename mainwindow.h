#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_sipher2plain_clicked();

    void on_pushButton_plain2sipher_clicked();

    void on_pushButton_plain2sipher_ascii_clicked();

    void on_pushButton_sipher2plain_ascii_clicked();

    void on_pushButton_browse_plain_clicked();
    void on_pushButton_browse_cipher_clicked();
    void on_pushButton_file_bruteforce_clicked();

private:
    Ui::MainWindow *ui;
    void applyModernStyle();
    void initializeUI();
};
#endif // MAINWINDOW_H
