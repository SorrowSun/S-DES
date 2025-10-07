#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QString>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QtMath>
#include <QMutex>
#include <QThread>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QElapsedTimer>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 设置窗口标题和图标
    setWindowTitle("S-DES 加解密工具");
    setMinimumSize(820, 500);
    
    ui->statusbar->showMessage("S-DES 加解密工具已就绪 | 支持二进制、ASCII字符串加解密和暴力破解密钥 | 作者：姜昊男，张俊杰", 0);
    
    initializeUI();
    
    // 应用现代化样式表
    applyModernStyle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QList<int>getKey(QString key, const QList<QList<int>>& leftShiftList, int pitch)
{
    QList<int>P10 = {2, 4, 1, 6, 3, 9, 0, 8, 7, 5};
    QList<int>tempKey = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 10; i++)
    {
        tempKey[i] = key[P10[i]].digitValue();
    }
    //进行P10转换

    QList<int>left = {};
    QList<int>right = {};
    for(int i = 0; i < 10; i++)
    {
        if(i < 5)
        {
            left.append(tempKey[i]);
        }
        else
        {
            right.append(tempKey[i]);
        }
    }
    //拆分成左右两部分的数组

    QList<int>leftShift = leftShiftList[pitch];
    QList<int>tempLeft(left);
    QList<int>tempRight(right);
    for(int i = 0; i < 5; i++)
    {
        left[i] = tempLeft[leftShift[i]];
        right[i] = tempRight[leftShift[i]];
    }
    //事实上是向左移位pitch+1位

    QList<int>P8 = {5, 2, 6, 3, 7, 4, 9, 8};
    QList<int>tempResult(left + right);
    QList<int>result = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        result[i] = tempResult[P8[i]];
    }
    return result;
    //进行P8转换
}

QList<int>switchRight(const QList<int>& right, const QList<int>& key)
{
    QList<int>EPBox = {3, 0, 1, 2, 1, 2, 3, 0};
    QList<int>R = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        R[i] = right[EPBox[i]];
    }
    //进行EPBox转换

    for(int i = 0; i < 8; i++)
    {
        if(R[i] == key[i])
        {
            R[i] = 0;
        }
        else
        {
            R[i] = 1;
        }
    }
    //R与key进行异或

    QList<int>tempR = {0, 0, 0, 0};
    QList<QList<int>>SBox1 = {{1, 0, 3, 2}, {3, 2, 1, 0}, {0, 2, 1, 3}, {3, 1, 0, 2}};
    QList<QList<int>>SBox2 = {{0, 1, 2, 3}, {2, 3, 1, 0}, {3, 0, 1, 2}, {2, 1, 0, 3}};

    int SBox1Horizonal = R[0] * 2 + R[3];
    int SBox1Virtical = R[1] * 2 + R[2];
    int SBox2Horizonal = R[4] * 2 + R[7];
    int SBox2Virtical = R[5] * 2 + R[6];
    int SBox1Result = SBox1[SBox1Horizonal][SBox1Virtical];
    int SBox2Result = SBox2[SBox2Horizonal][SBox2Virtical];

    if(SBox1Result == 0){ tempR[0] = 0; tempR[1] = 0;}
    else if(SBox1Result == 1){ tempR[0] = 0; tempR[1] = 1;}
    else if(SBox1Result == 2){ tempR[0] = 1; tempR[1] = 0;}
    else{ tempR[0] = 1; tempR[1] = 1;}

    if(SBox2Result == 0){ tempR[2] = 0; tempR[3] = 0;}
    else if(SBox2Result == 1){ tempR[2] = 0; tempR[3] = 1;}
    else if(SBox2Result == 2){ tempR[2] = 1; tempR[3] = 0;}
    else{ tempR[2] = 1; tempR[3] = 1;}
    //进行SBox转换

    QList<int>SPBox = {1, 3, 2, 0};
    QList<int>result(tempR);
    for(int i = 0; i < 4; i++)
    {
        result[i] = tempR[SPBox[i]];
    }
    return result;
    //最后进行SPBox转换，return
}

QString plain2sipher(QString plain, const QList<int>& key1, const QList<int>& key2)
{
    QList<int>IP = {1, 5, 2, 0, 3, 7, 4, 6};
    QList<int>tempPlain = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        tempPlain[i] = plain[IP[i]].digitValue();
    }
    //进行IP转换

    QList<int>left = {};
    QList<int>right = {};
    for(int i = 0; i < 8; i++)
    {
        if(i < 4)
        {
            left.append(tempPlain[i]);
        }
        else
        {
            right.append(tempPlain[i]);
        }
    }
    //拆分成左右两组

    QList<int>R1(switchRight(right, key1));
    //qDebug() << R1;
    QList<int>L1(left);
    for(int i = 0; i < 4; i++)
    {
        if(R1[i] == L1[i])
        {
            L1[i] = 0;
        }
        else
        {
            L1[i] = 1;
        }
    }
    //qDebug() << L1;
    //第一轮转换R之后，用R对L进行异或操作

    QList<int>R2(L1);
    QList<int>L2(right);
    //swap左右

    QList<int>R3(switchRight(R2, key2));
    QList<int>L3(L2);
    for(int i = 0; i < 4; i++)
    {
        if(R3[i] == L3[i])
        {
            L3[i] = 0;
        }
        else
        {
            L3[i] = 1;
        }
    }
    //第二轮转换R之后，用R对L进行异或操作

    QList<int>IP_ = {3, 0, 2, 4, 6, 1, 7, 5};
    QList<int>tempResult(L3 + R2);
    QList<int>listResult = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        listResult[i] = tempResult[IP_[i]];
    }
    //最后进行IP_的置换

    QString result;
    QTextStream stream(&result);
    for(int i = 0; i < 8; i++)
    {
        stream << listResult[i];
    }
    stream.flush();
    return result;
    //最后把QList转化成QString
}

QString sipher2plain(QString sipher, const QList<int>& key1, const QList<int>& key2)
{
    QList<int>IP = {1, 5, 2, 0, 3, 7, 4, 6};
    QList<int>tempSipher = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        tempSipher[i] = sipher[IP[i]].digitValue();
    }
    //进行IP转换

    QList<int>left = {};
    QList<int>right = {};
    for(int i = 0; i < 8; i++)
    {
        if(i < 4)
        {
            left.append(tempSipher[i]);
        }
        else
        {
            right.append(tempSipher[i]);
        }
    }
    //拆分成左右两组

    QList<int>R1(switchRight(right, key2));
    //qDebug() << R1;
    QList<int>L1(left);
    for(int i = 0; i < 4; i++)
    {
        if(R1[i] == L1[i])
        {
            L1[i] = 0;
        }
        else
        {
            L1[i] = 1;
        }
    }
    //qDebug() << L1;
    //第一轮转换R之后，用R对L进行异或操作

    QList<int>R2(L1);
    QList<int>L2(right);
    //swap左右

    QList<int>R3(switchRight(R2, key1));
    QList<int>L3(L2);
    for(int i = 0; i < 4; i++)
    {
        if(R3[i] == L3[i])
        {
            L3[i] = 0;
        }
        else
        {
            L3[i] = 1;
        }
    }
    //第二轮转换R之后，用R对L进行异或操作

    QList<int>IP_ = {3, 0, 2, 4, 6, 1, 7, 5};
    QList<int>tempResult(L3 + R2);
    QList<int>listResult = {0, 0, 0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 8; i++)
    {
        listResult[i] = tempResult[IP_[i]];
    }
    //最后进行IP的置换

    QString result;
    QTextStream stream(&result);
    for(int i = 0; i < 8; i++)
    {
        stream << listResult[i];
    }
    stream.flush();
    return result;
    //最后把QList转化成QString
}

void MainWindow::on_pushButton_sipher2plain_clicked()
{
    QString sipherMessage = ui->lineEdit_sipher->text();
    QString keyMessage = ui->lineEdit_key->text();
    //获取密文和密钥

    if(sipherMessage.length() != 8 || keyMessage.length() != 10)
    {
        QMessageBox::information(nullptr, "提示", "密文或者密钥长度不匹配", QMessageBox::Ok);
        return;
    }
    //长度不符合警告

    QRegularExpression regex("^[01]*$");
    bool isBinary = (regex.match(sipherMessage).hasMatch()) && (regex.match(keyMessage).hasMatch());
    if(!isBinary)
    {
        QMessageBox::information(nullptr, "提示", "密文或者密钥不是二进制表示格式", QMessageBox::Ok);
        return;
    }
    //非二进制表示格式警告

    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyMessage, leftShift, 0);
    //qDebug() << key1;
    QList<int>key2 = getKey(keyMessage, leftShift, 1);
    //qDebug() << key2;
    //获取两段密钥

    QString plainMessage = sipher2plain(sipherMessage, key1, key2);
    ui->lineEdit_plain->setText(plainMessage);
    
    // 显示成功状态
    ui->statusbar->showMessage(QString("二进制解密成功: %1 → %2").arg(sipherMessage).arg(plainMessage), 3000);
    //转换并显示
}

void MainWindow::on_pushButton_plain2sipher_clicked()
{
    QString plainMessage = ui->lineEdit_plain->text();
    QString keyMessage = ui->lineEdit_key->text();
    //获取明文和密钥

    if(plainMessage.length() != 8 || keyMessage.length() != 10)
    {
        QMessageBox::information(nullptr, "提示", "明文或者密钥长度不匹配", QMessageBox::Ok);
        return;
    }
    //长度不符合警告

    QRegularExpression regex("^[01]*$");
    bool isBinary = (regex.match(plainMessage).hasMatch()) && (regex.match(keyMessage).hasMatch());
    if(!isBinary)
    {
        QMessageBox::information(nullptr, "提示", "明文或者密钥不是二进制表示格式", QMessageBox::Ok);
        return;
    }
    //非二进制表示格式警告

    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyMessage, leftShift, 0);
    //qDebug() << key1;
    QList<int>key2 = getKey(keyMessage, leftShift, 1);
    //qDebug() << key2;
    //获取两段密钥

    QString sipherMessage = plain2sipher(plainMessage, key1, key2);
    ui->lineEdit_sipher->setText(sipherMessage);
    
    // 显示成功状态
    ui->statusbar->showMessage(QString("二进制加密成功: %1 → %2").arg(plainMessage).arg(sipherMessage), 3000);
    //转换并显示
}


void MainWindow::on_pushButton_plain2sipher_ascii_clicked()
{
    QString plainMessage = ui->lineEdit_plain_ascii->text();
    QString keyMessage = ui->lineEdit_key_ascii->text();

    if(keyMessage.length() != 10)
    {
        QMessageBox::information(nullptr, "提示", "密钥长度不匹配", QMessageBox::Ok);
        return;
    }
    //长度不符合警告

    QRegularExpression regex_binary("^[01]*$");
    bool isBinary = regex_binary.match(keyMessage).hasMatch();
    if(!isBinary)
    {
        QMessageBox::information(nullptr, "提示", "密钥不是二进制表示格式", QMessageBox::Ok);
        return;
    }
    //非二进制表示格式警告

    QRegularExpression regex_ascii("^[\\x00-\\xFF]*$");
    bool isAscii = regex_ascii.match(plainMessage).hasMatch();
    if(!isAscii)
    {
        QMessageBox::information(nullptr, "提示", "明文不在ascii码表中", QMessageBox::Ok);
        return;
    }
    //非ascii码警告

    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyMessage, leftShift, 0);
    //qDebug() << key1;
    QList<int>key2 = getKey(keyMessage, leftShift, 1);
    //qDebug() << key2;
    //获取两段密钥

    QString result = "";
    QTextStream resultStream(&result);
    for(int i = 0; i < plainMessage.length(); i++)
    {
        QString binary = "";
        QTextStream binaryStream(&binary);
        double charValue = (unsigned char)plainMessage[i].toLatin1() + 0.0001;
        //加上0.0001是为了防止浮点数的精度导致出现问题
        for(int i = 7; i >= 0; i--)
        {
            double pow = qPow(2, i);
            if(charValue >= pow)
            {
                binaryStream << "1";
                charValue -= pow;
            }
            else
            {
                binaryStream << "0";
            }
        }
        binaryStream.flush();
        //qDebug() << binary;
        //分别转换成二进制QString格式

        QString resultBinary = plain2sipher(binary, key1, key2);
        //qDebug() << resultBinary;
        //进行二进制加密

        double asciiDoubleResult = 0;
        for(int i = 0; i < 8; i++)
        {
            if(resultBinary[i] == '1')
            {
                asciiDoubleResult += qPow(2, 7-i);
            }
        }
        int asciiResult = qRound(asciiDoubleResult);
        //算出最终结果的ascii码值

        resultStream << QChar(asciiResult);
    }
    resultStream.flush();
    ui->lineEdit_sipher_ascii->setText(result);
    
    // 显示成功状态
    ui->statusbar->showMessage(QString("ASCII加密成功: \"%1\" → \"%2\" (%3个字符)")
        .arg(plainMessage).arg(result).arg(result.length()), 5000);
    //按位转换然后输出
}


void MainWindow::on_pushButton_sipher2plain_ascii_clicked()
{
    QString sipherMessage = ui->lineEdit_sipher_ascii->text();
    QString keyMessage = ui->lineEdit_key_ascii->text();

    if(keyMessage.length() != 10)
    {
        QMessageBox::information(nullptr, "提示", "密钥长度不匹配", QMessageBox::Ok);
        return;
    }
    //长度不符合警告

    QRegularExpression regex_binary("^[01]*$");
    bool isBinary = regex_binary.match(keyMessage).hasMatch();
    if(!isBinary)
    {
        QMessageBox::information(nullptr, "提示", "密钥不是二进制表示格式", QMessageBox::Ok);
        return;
    }
    //非二进制表示格式警告

    QRegularExpression regex_ascii("^[\\x00-\\xFF]*$");
    bool isAscii = regex_ascii.match(sipherMessage).hasMatch();
    if(!isAscii)
    {
        QMessageBox::information(nullptr, "提示", "密文不在ascii码表中", QMessageBox::Ok);
        return;
    }
    //非ascii码警告

    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyMessage, leftShift, 0);
    //qDebug() << key1;
    QList<int>key2 = getKey(keyMessage, leftShift, 1);
    //qDebug() << key2;
    //获取两段密钥

    QString result = "";
    QTextStream resultStream(&result);
    for(int i = 0; i < sipherMessage.length(); i++)
    {
        QString binary = "";
        QTextStream binaryStream(&binary);
        double charValue = (unsigned char)sipherMessage[i].toLatin1() + 0.0001;
        //加上0.0001是为了防止浮点数的精度导致出现问题
        for(int i = 7; i >= 0; i--)
        {
            double pow = qPow(2, i);
            if(charValue >= pow)
            {
                binaryStream << "1";
                charValue -= pow;
            }
            else
            {
                binaryStream << "0";
            }
        }
        binaryStream.flush();
        qDebug() << binary;
        //分别转换成二进制QString格式

        QString resultBinary = sipher2plain(binary, key1, key2);
        // qDebug() << resultBinary;
        //进行二进制加密

        double asciiDoubleResult = 0;
        for(int i = 0; i < 8; i++)
        {
            if(resultBinary[i] == '1')
            {
                asciiDoubleResult += qPow(2, 7-i);
            }
        }
        int asciiResult = qRound(asciiDoubleResult);
        //算出最终结果的ascii码值

        resultStream << QChar(asciiResult);
    }
    resultStream.flush();
    ui->lineEdit_plain_ascii->setText(result);
    
    // 显示成功状态
    ui->statusbar->showMessage(QString("ASCII解密成功: \"%1\" → \"%2\" (%3个字符)")
        .arg(sipherMessage).arg(result).arg(result.length()), 5000);
    //按位转换然后输出
}


int found = 0;
QMutex mutex;
int result_key[10];
int current_progress = 0;

void add(int key[], int n)
{
    if (n >= 10)
        return;
    if (key[n] == 0)
    {
        key[n] = 1;
    }
    else
    {
        key[n] = 0;
        add(key, n + 1);
    }
}


QString decode(const QString& ciphertext, const int key[]) {
    // 将int数组转换为QString格式的密钥
    QString keyStr = "";
    QTextStream keyStream(&keyStr);
    for(int i = 0; i < 10; i++) {
        keyStream << key[i];
    }
    keyStream.flush();
    
    // 生成子密钥
    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyStr, leftShift, 0);
    QList<int>key2 = getKey(keyStr, leftShift, 1);
    
    return sipher2plain(ciphertext, key1, key2);
}




// 字符串转二进制
QString stringToBinary(const QString& str) {
    QString result = "";
    QTextStream resultStream(&result);
    
    for(int i = 0; i < str.length(); i++) {
        QString binary = "";
        QTextStream binaryStream(&binary);
        double charValue = (unsigned char)str[i].toLatin1() + 0.0001;
        
        for(int j = 7; j >= 0; j--) {
            double pow = qPow(2, j);
            if(charValue >= pow) {
                binaryStream << "1";
                charValue -= pow;
            } else {
                binaryStream << "0";
            }
        }
        binaryStream.flush();
        resultStream << binary;
    }
    resultStream.flush();
    return result;
}

// 字符串解密函数
QString decodeString(const QString& ciphertextStr, const int key[]) {
    QString keyStr = "";
    QTextStream keyStream(&keyStr);
    for(int i = 0; i < 10; i++) {
        keyStream << key[i];
    }
    keyStream.flush();
    
    QList<QList<int>>leftShift = {{1, 2, 3, 4, 0}, {2, 3, 4, 0, 1}};
    QList<int>key1 = getKey(keyStr, leftShift, 0);
    QList<int>key2 = getKey(keyStr, leftShift, 1);
    
    QString result = "";
    QTextStream resultStream(&result);
    
    // 将字符串转为二进制
    QString cipherBinary = stringToBinary(ciphertextStr);
    
    // 每8位进行一次解密
    for(int i = 0; i < cipherBinary.length(); i += 8) {
        if(i + 7 < cipherBinary.length()) {
            QString charCipher = cipherBinary.mid(i, 8);
            QString charPlain = sipher2plain(charCipher, key1, key2);
            
            // 转换回字符
            double asciiValue = 0;
            for(int j = 0; j < 8; j++) {
                if(charPlain[j] == '1') {
                    asciiValue += qPow(2, 7-j);
                }
            }
            int asciiResult = qRound(asciiValue);
            resultStream << QChar(asciiResult);
        }
    }
    resultStream.flush();
    return result;
}



// 选择明文文件
void MainWindow::on_pushButton_browse_plain_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择明文文件"), "",
        tr("文本文件 (*.txt);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        ui->lineEdit_file_plain->setText(fileName);
    }
}

// 选择密文文件
void MainWindow::on_pushButton_browse_cipher_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("选择密文文件"), "",
        tr("文本文件 (*.txt);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        ui->lineEdit_file_cipher->setText(fileName);
    }
}

// 读取文件内容到字符串列表
QStringList readFileToList(const QString& filePath) {
    QStringList result;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result; // 返回空列表表示读取失败
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8); // 设置编码为UTF-8
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed(); // 去除行首行尾空白
        if (!line.isEmpty()) { // 只添加非空行
            result.append(line);
        }
    }
    
    file.close();
    return result;
}

// 检测内容类型：二进制还是字符串
bool isBinaryContent(const QString& content) {
    // 如果内容只包含0和1，且长度是8的倍数，认为是二进制
    QRegularExpression binaryRegex("^[01]+$");
    return binaryRegex.match(content).hasMatch() && (content.length() % 8 == 0);
}

// 批量暴力破解
void solveBatch(int start, int end, const QStringList& plaintextList, const QStringList& ciphertextList, bool isBinary) {
    int key[10] = {0};
    
    // 初始化到起始位置
    for (int i = 0; i < start; ++i) {
        add(key, 0);
    }
    
    for (int i = start; i < end; ++i) {
        mutex.lock();
        if (found == 1) {
            mutex.unlock();
            return;
        }
        current_progress = i;
        mutex.unlock();

        // 测试当前密钥是否能正确解密所有明文密文对
        bool keyMatches = true;
        
        for (int j = 0; j < plaintextList.size(); ++j) {
            QString expectedPlain = plaintextList[j];
            QString cipher = ciphertextList[j];
            QString decryptedPlain;
            
            if (isBinary) {
                // 二进制模式：每8位一组进行解密
                QString fullDecrypted = "";
                for (int k = 0; k < cipher.length(); k += 8) {
                    if (k + 7 < cipher.length()) {
                        QString block = cipher.mid(k, 8);
                        fullDecrypted += decode(block, key);
                    }
                }
                decryptedPlain = fullDecrypted;
            } else {
                // 字符串模式
                decryptedPlain = decodeString(cipher, key);
            }
            
            if (decryptedPlain != expectedPlain) {
                keyMatches = false;
                break;
            }
        }
        
        if (keyMatches) {
            mutex.lock();
            if (found == 0) {
                found = 1;
                for(int j = 0; j < 10; j++) {
                    result_key[j] = key[j];
                }
            }
            mutex.unlock();
            return;
        }
        
        add(key, 0);
    }
}

// 文件批量暴力破解UI
void MainWindow::on_pushButton_file_bruteforce_clicked()
{
    QString plainFilePath = ui->lineEdit_file_plain->text();
    QString cipherFilePath = ui->lineEdit_file_cipher->text();
    
    // 检查文件路径
    if (plainFilePath.isEmpty() || cipherFilePath.isEmpty()) {
        QMessageBox::information(this, "提示", 
            "请选择明文文件和密文文件", QMessageBox::Ok);
        return;
    }
    
    // 读取文件内容
    QStringList plaintextList = readFileToList(plainFilePath);
    QStringList ciphertextList = readFileToList(cipherFilePath);
    
    // 检查文件读取结果
    if (plaintextList.isEmpty()) {
        QMessageBox::information(this, "错误", 
            QString("无法读取明文文件: %1\n\n请检查文件是否存在且可读").arg(plainFilePath), 
            QMessageBox::Ok);
        return;
    }
    
    if (ciphertextList.isEmpty()) {
        QMessageBox::information(this, "错误", 
            QString("无法读取密文文件: %1\n\n请检查文件是否存在且可读").arg(cipherFilePath), 
            QMessageBox::Ok);
        return;
    }
    
    // 检查行数是否匹配
    if (plaintextList.size() != ciphertextList.size()) {
        QMessageBox::information(this, "错误", 
            QString("文件行数不匹配:\n明文文件: %1行\n密文文件: %2行\n\n请确保两个文件的行数相同")
            .arg(plaintextList.size()).arg(ciphertextList.size()), 
            QMessageBox::Ok);
        return;
    }
    
    // 检测内容类型（二进制或字符串）
    bool isBinary = true;
    for (const QString& line : plaintextList) {
        if (!isBinaryContent(line)) {
            isBinary = false;
            break;
        }
    }
    
    if (isBinary) {
        for (const QString& line : ciphertextList) {
            if (!isBinaryContent(line)) {
                isBinary = false;
                break;
            }
        }
    }
    
    // 显示检测结果并确认
    QString contentType = isBinary ? "二进制" : "字符串";
    int ret = QMessageBox::question(this, "确认破解", 
        QString("检测到文件内容为: %1\n\n文件信息:\n- 明文行数: %2\n- 密文行数: %3\n- 内容类型: %4\n\n确定要开始暴力破解吗？")
        .arg(contentType).arg(plaintextList.size()).arg(ciphertextList.size()).arg(contentType),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 重置全局变量和界面
    found = 0;
    current_progress = 0;
    for(int i = 0; i < 10; i++) {
        result_key[i] = 0;
    }
    
    // 初始化进度条和时间显示
    ui->progressBar_bruteforce->setMaximum(1024);
    ui->progressBar_bruteforce->setValue(0);
    ui->lineEdit_result_key->clear();
    
    // 启动计时
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    
    // 创建定时器更新界面
    QTimer *updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, [=]() {
        // 更新进度条
        mutex.lock();
        int currentProg = current_progress;
        mutex.unlock();
        
        ui->progressBar_bruteforce->setValue(currentProg);
        
        // 更新时间显示
        qint64 elapsed = elapsedTimer.elapsed();
        QString timeText;
        if (elapsed < 60000) {
            timeText = QString("⏱️ 用时: %1秒").arg(elapsed / 1000.0, 0, 'f', 1);
        } else {
            int minutes = elapsed / 60000;
            int seconds = (elapsed % 60000) / 1000;
            timeText = QString("⏱️ 用时: %1分%2秒").arg(minutes).arg(seconds);
        }
        ui->label_time_display->setText(timeText);
        
        // 如果破解完成或找到密钥，停止定时器
        if (found == 1 || currentProg >= 1024) {
            updateTimer->stop();
            updateTimer->deleteLater();
        }
    });
    updateTimer->start(100); // 每100ms更新一次
    
    // 禁用破解按钮
    ui->pushButton_file_bruteforce->setEnabled(false);
    ui->pushButton_file_bruteforce->setText("⏳ 正在破解中...");
    
    // 多线程暴力破解
    const int totalKeys = 1024;
    const int threadCount = QThread::idealThreadCount();
    const int keysPerThread = totalKeys / threadCount;
    
    QList<QFuture<void>> futures;
    
    for(int i = 0; i < threadCount; i++) {
        int start = i * keysPerThread;
        int end = (i == threadCount - 1) ? totalKeys : (i + 1) * keysPerThread;
        
        QFuture<void> future = QtConcurrent::run([=]() {
            solveBatch(start, end, plaintextList, ciphertextList, isBinary);
        });
        futures.append(future);
    }
    
    // 等待所有线程完成
    for(auto& future : futures) {
        future.waitForFinished();
    }
    
    // 停止定时器并恢复按钮
    updateTimer->stop();
    updateTimer->deleteLater();
    ui->pushButton_file_bruteforce->setEnabled(true);
    ui->pushButton_file_bruteforce->setText("🚀 开始文件批量破解");
    
    // 更新进度条和时间
    ui->progressBar_bruteforce->setValue(1024);
    qint64 elapsed = elapsedTimer.elapsed();
    QString finalTimeText;
    if (elapsed < 60000) {
        finalTimeText = QString(" 用时: %1秒").arg(elapsed / 1000.0, 0, 'f', 1);
    } else {
        int minutes = elapsed / 60000;
        int seconds = (elapsed % 60000) / 1000;
        finalTimeText = QString(" 用时: %1分%2秒").arg(minutes).arg(seconds);
    }
    ui->label_time_display->setText(finalTimeText);
    
    // 检查结果
    if(found == 1) {
        QString foundKey = "";
        QTextStream keyStream(&foundKey);
        for(int i = 0; i < 10; i++) {
            keyStream << result_key[i];
        }
        keyStream.flush();
        
        ui->lineEdit_result_key->setText(foundKey);
        
        QMessageBox::information(this, "批量破解成功",
            QString("找到密钥: %1\n\n 批量破解统计:\n- 数据组数: %2\n- 内容类型: %3\n- 使用线程: %4个\n- %5\n\n ")
            .arg(foundKey).arg(plaintextList.size()).arg(contentType).arg(threadCount).arg(finalTimeText), 
            QMessageBox::Ok);
    }
    else {
        QMessageBox::information(this, "批量破解失败",
            QString("未能找到匹配的密钥")
            .arg(finalTimeText), 
            QMessageBox::Ok);
    }
}


void MainWindow::applyModernStyle()
{
    QString styleSheet = R"(
/* 主窗口样式 */
QMainWindow {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #f8f9fa, stop:1 #e9ecef);
}

/* 标签页样式 */
QTabWidget::pane {
    border: 2px solid #dee2e6;
    border-radius: 8px;
    background: white;
    margin-top: -1px;
}

QTabWidget::tab-bar {
    alignment: center;
}

QTabBar::tab {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #ffffff, stop:1 #f8f9fa);
    border: 2px solid #dee2e6;
    border-bottom-color: #dee2e6;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    min-width: 120px;
    padding: 12px 20px;
    margin: 2px;
    font-weight: 500;
    font-size: 11px;
}

QTabBar::tab:selected {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #007bff, stop:1 #0056b3);
    color: white;
    border-bottom-color: white;
    font-weight: 600;
}

QTabBar::tab:!selected {
    margin-top: 4px;
    color: #6c757d;
}

QTabBar::tab:hover:!selected {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #e3f2fd, stop:1 #bbdefb);
    color: #1976d2;
}

/* 按钮样式 */
QPushButton {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #007bff, stop:1 #0056b3);
    color: white;
    border: none;
    border-radius: 8px;
    padding: 12px 24px;
    font-weight: 600;
    font-size: 11px;
    min-height: 16px;
}

QPushButton:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #0056b3, stop:1 #004085);
    transform: translateY(-1px);
}

QPushButton:pressed {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #004085, stop:1 #002752);
}

QPushButton:disabled {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #6c757d, stop:1 #5a6268);
    color: #adb5bd;
}

/* 浏览按钮特殊样式 */
QPushButton#pushButton_browse_plain,
QPushButton#pushButton_browse_cipher {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #28a745, stop:1 #1e7e34);
    min-width: 80px;
}

QPushButton#pushButton_browse_plain:hover,
QPushButton#pushButton_browse_cipher:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1e7e34, stop:1 #155724);
}

/* 文件破解按钮特殊样式 */
QPushButton#pushButton_file_bruteforce {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #dc3545, stop:1 #c82333);
    font-size: 12px;
    font-weight: 700;
    min-width: 160px;
    min-height: 20px;
}

QPushButton#pushButton_file_bruteforce:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #c82333, stop:1 #bd2130);
}

/* 输入框样式 */
QLineEdit {
    border: 2px solid #dee2e6;
    border-radius: 6px;
    padding: 10px 12px;
    font-size: 11px;
    background: white;
    selection-background-color: #007bff;
}

QLineEdit:focus {
    border-color: #007bff;
    box-shadow: 0 0 0 3px rgba(0, 123, 255, 0.25);
}

QLineEdit:read-only {
    background: #f8f9fa;
    color: #6c757d;
}

/* 结果密钥输入框特殊样式 */
QLineEdit#lineEdit_result_key {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #d4edda, stop:1 #c3e6cb);
    border: 2px solid #28a745;
    font-family: 'Consolas', 'Monaco', monospace;
    font-size: 14px;
    font-weight: 600;
    color: #155724;
    padding: 12px 16px;
    min-height: 24px;
}

/* 标签样式 */
QLabel {
    color: #343a40;
    font-weight: 500;
    font-size: 11px;
}

QLabel#label_brute_info {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #f8f9fa, stop:1 #e9ecef);
    border: 1px solid #dee2e6;
    border-radius: 8px;
    padding: 16px;
    color: #495057;
    font-size: 10px;
    line-height: 1.4;
}

/* 进度条样式 */
QProgressBar {
    border: 2px solid #dee2e6;
    border-radius: 8px;
    text-align: center;
    font-weight: 600;
    font-size: 11px;
    color: white;
    background: #f8f9fa;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #007bff, stop:0.5 #0056b3, stop:1 #004085);
    border-radius: 6px;
    margin: 2px;
}

/* 时间显示标签特殊样式 */
QLabel#label_time_display {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #fff3cd, stop:1 #ffeaa7);
    border: 1px solid #ffc107;
    border-radius: 6px;
    padding: 8px 12px;
    color: #856404;
    font-weight: 600;
    font-family: 'Consolas', 'Monaco', monospace;
}

QLabel#label_progress {
    color: #495057;
    font-weight: 600;
}

/* 状态栏样式 */
QStatusBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #ffffff, stop:1 #f8f9fa);
    border-top: 1px solid #dee2e6;
    color: #6c757d;
    font-size: 10px;
}
)";

    this->setStyleSheet(styleSheet);
}

// 初始化UI
void MainWindow::initializeUI()
{
    // 设置进度条初始状态
    ui->progressBar_bruteforce->setVisible(true);
    ui->progressBar_bruteforce->setValue(0);
    ui->progressBar_bruteforce->setFormat("准备就绪 - 等待开始破解");
    
    // 设置结果密钥框的初始状态
    ui->lineEdit_result_key->setPlaceholderText("破解成功后会在这里显示找到的10位二进制密钥");
    
    // 确保密钥输入框高度
    ui->lineEdit_result_key->setMinimumHeight(40);
    ui->lineEdit_result_key->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // 为文件路径输入框提示
    ui->lineEdit_file_plain->setPlaceholderText("选择明文txt文件（每行一个明文，支持字符串或二进制格式）");
    ui->lineEdit_file_cipher->setPlaceholderText("选择密文txt文件（每行一个密文，与明文文件行数匹配）");
    
    // 设置标签页的提示信息
    ui->tabWidget->setTabToolTip(0, "进行8位二进制数据的S-DES加密和解密操作");
    ui->tabWidget->setTabToolTip(1, "进行ASCII字符串的S-DES加密和解密操作，支持任意长度文本");
    ui->tabWidget->setTabToolTip(2, "使用已知明文密文对进行智能暴力破解，找出正确的密钥");
}
