#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Interface_Restaurant.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Interface_Restaurant* dashboard = new Interface_Restaurant();
    dashboard->show();

    // Cacher la fenêtre MainWindow vide
    this->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}