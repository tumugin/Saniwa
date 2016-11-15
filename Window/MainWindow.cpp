﻿#include <iostream>
#include <thread>
#include <QFileDialog>
#include <QMessageBox>
#include "MainWindow.h"
#include "ui_MainWindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::MainWindow){
    ui->setupUi(this);
    this->setWindowTitle(u8"Saniwa");
    connect(ui->webEngineView,SIGNAL(loadFinished(bool)),this,SLOT(onBrowserLoadFinish(bool)));
    ui->webEngineView->setUrl(QUrl("http://www.dmm.com/netgame/social/-/gadgets/=/app_id=825012/"));
    wRThread = std::unique_ptr<std::thread>(new std::thread(&MainWindow::windowResizeCheck,this));
    //init statusbar button
    sShotButton = std::unique_ptr<QToolButton>(new QToolButton(ui->statusbar));
    sShotButton->setText(u8"スクショ");
    ui->statusbar->addWidget(sShotButton.get());
    connect(sShotButton.get(),SIGNAL(clicked()),this,SLOT(saveScreenShot()));
    logoutButton = std::unique_ptr<QToolButton>(new QToolButton(ui->statusbar));
    logoutButton->setText(u8"ログアウト");
    ui->statusbar->addWidget(logoutButton.get());
    connect(logoutButton.get(),SIGNAL(clicked()),this,SLOT(logoutDMM()));
    reloadButton = std::unique_ptr<QToolButton>(new QToolButton(ui->statusbar));
    reloadButton->setText(u8"リロード");
    ui->statusbar->addWidget(reloadButton.get());
    connect(reloadButton.get(),SIGNAL(clicked()),this,SLOT(reloadPage()));
#ifndef __unix
    //not working on X11 env
    onTopButton = std::unique_ptr<QToolButton>(new QToolButton(ui->statusbar));
    onTopButton->setText(u8"最上面に表示");
    onTopButton->setCheckable(true);
    ui->statusbar->addWidget(onTopButton.get());
    connect(onTopButton.get(),SIGNAL(toggled(bool)),this,SLOT(enableOnTop(bool)));
#endif
	muteButton = std::unique_ptr<QToolButton>(new QToolButton(ui->statusbar));
	muteButton->setText(u8"消音");
	muteButton->setCheckable(true);
	ui->statusbar->addWidget(muteButton.get());
    cout << "window initialized" << endl;
}

MainWindow::~MainWindow() {
    stopAllWindowThread = true;
    wRThread->detach();
}

void MainWindow::onBrowserLoadFinish(bool stat) {
    std::string url = ui->webEngineView->url().toString().toStdString();
    cout << "Load finished. " << url << endl;
    if(url == "http://www.dmm.com/netgame/social/-/gadgets/=/app_id=825012/"){
        tokenLoadOK = true;
        cout << "Game page load OK." << endl;
        ui->webEngineView->page()->runJavaScript("$(\"#game_frame\").attr(\"src\")",[this](const QVariant &v) {
            if(v.isNull() || v.toString().isEmpty()) return;
            cout << "iframe url " << v.toString().toStdString() << endl;
            ui->webEngineView->setUrl(QUrl(v.toString()));
        });
    }else if(tokenLoadOK && ui->webEngineView->url().authority().toStdString() == "osapi.dmm.com"){
        std::thread th([this](){
            cout << "sleep start" << endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            //call slot on main window thread
            QMetaObject::invokeMethod(this,"onBrowserWaitComplete");
        });
        th.detach();
    }else if(url == "http://www.dmm.com/"){
        ui->webEngineView->setUrl(QUrl("http://www.dmm.com/netgame/social/-/gadgets/=/app_id=825012/"));
    }
}

void MainWindow::onBrowserWaitComplete() {
    if(ui == nullptr) return;
    ui->webEngineView->page()->runJavaScript("document.body.style.overflow = \"hidden\"");
    ui->webEngineView->page()->runJavaScript("$(function(){$('#flash_object').css({'position': 'fixed','left': '0','width': '100%','height': '100%','z-index': '1000000'})});",[this](const QVariant &v){
        cout << v.toString().toStdString() << endl;
    });
    this->resize(960,580 + this->ui->statusbar->height());
}

void MainWindow::onWindowResized() {
    double resizedHeight = (this->size().width() / 960.0) * (580.0 + (double)this->ui->statusbar->height());
    this->resize(this->size().width(),(int)resizedHeight);
}

void MainWindow::windowResizeCheck() {
    QSize lastSize;
    while (!stopAllWindowThread){
        lastSize = this->size();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(tokenLoadOK && (lastSize.width() != this->size().width() || lastSize.height() != this->size().height())){
            //call slot on main window thread
            QMetaObject::invokeMethod(this,"onWindowResized");
        }
    }
}

void MainWindow::saveScreenShot() {
    cout << "took screenshot" << endl;
    QPixmap p = QPixmap::grabWidget(ui->webEngineView);
    QString savepath = QFileDialog::getSaveFileName(this,u8"スクショを保存","","PNG (*.png)");
    if(savepath.isEmpty()) return;
    if(!savepath.endsWith(".png")) savepath += ".png";
    std::thread([=]{
        p.save(savepath);
    }).join();
}

void MainWindow::logoutDMM() {
    tokenLoadOK = false;
    if(QMessageBox::question(this,"Saniwa",u8"本当に本丸からログアウトしますか?",QMessageBox::Yes | QMessageBox::No,QMessageBox::No) == QMessageBox::Yes){
        ui->webEngineView->setUrl(QUrl(u8"https://www.dmm.com/my/-/login/logout/=/path=Sg__/"));
    }
}

void MainWindow::reloadPage() {
    if(QMessageBox::question(this,"Saniwa",u8"リロードしますか?",QMessageBox::Yes | QMessageBox::No,QMessageBox::No) == QMessageBox::Yes){
        ui->webEngineView->reload();
    }
}

void MainWindow::enableOnTop(bool toggle) {
    if(!toggle){
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnTopHint);
        this->show();
    }else{
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        this->show();
    }
}

void MainWindow::enableMute(bool toggle) {
	toggle ? ui->webEngineView->page()->setAudioMuted(true) : ui->webEngineView->page()->setAudioMuted(false);
}