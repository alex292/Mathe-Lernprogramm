#include "mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      random_engine_(random_device_()),
      multiplication_distribution_(1, 10),
      add_distribution_(1, 1000) {
  ui->setupUi(this);
  ui->tableWidget_statistics->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  ui->tableWidget_statistics->horizontalHeader()->setStretchLastSection(true);
  ui->tableWidget_statistics->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui->tableWidget_history->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tableWidget_history->horizontalHeader()->setStretchLastSection(true);

  operation_infos_.push_back(OperationInfo(Operation::Add, "+"));
  operation_infos_.push_back(OperationInfo(Operation::Subtract, "-"));
  operation_infos_.push_back(OperationInfo(Operation::Multiply, "×"));
  operation_infos_.push_back(OperationInfo(Operation::Divide, "÷"));

  QStringList operation_symbols;
  for (int i = 0; i < operation_infos_.size(); ++i) {
    const OperationInfo &op = operation_infos_[i];
    operation_symbols.append(op.symbol);
    operation_indices_.insert({op.operation, i});

    ui->tableWidget_statistics->insertRow(i);
    ui->tableWidget_statistics->setItem(i, 0, new QTableWidgetItem("0"));
    ui->tableWidget_statistics->setItem(i, 1, new QTableWidgetItem("0"));
    ui->tableWidget_statistics->setItem(i, 2, new QTableWidgetItem("0:00"));
  }
  ui->tableWidget_statistics->setVerticalHeaderLabels(operation_symbols);

  input_default_palette_ = ui->spinBox_solution->palette();
  input_error_palette_ = ui->spinBox_solution->palette();
  input_error_palette_.setColor(QPalette::Base, QColor(255, 70, 70));

  GenerateNewCurrentTask();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::GenerateNewCurrentTask() {
  // generate task
  std::shared_ptr<Task> task = std::make_shared<Task>();

  std::uniform_int_distribution<int> operation_distribution(0, operation_infos_.size() - 1);
  int operation_index = operation_distribution(random_engine_);
  task->operation = operation_infos_[operation_index].operation;

  switch (task->operation) {
    case Operation::Add: {
      task->value_a = add_distribution_(random_engine_);
      task->value_b = add_distribution_(random_engine_);
      task->result = task->value_a + task->value_b;
      break;
    }
    case Operation::Subtract: {
      task->value_a = add_distribution_(random_engine_);
      task->value_b = add_distribution_(random_engine_);
      task->result = task->value_a - task->value_b;
      break;
    }
    case Operation::Multiply: {
      task->value_a = multiplication_distribution_(random_engine_);
      task->value_b = multiplication_distribution_(random_engine_);
      task->result = task->value_a * task->value_b;
      break;
    }
    case Operation::Divide: {
      task->result = multiplication_distribution_(random_engine_);
      task->value_b = multiplication_distribution_(random_engine_);
      task->value_a = task->result * task->value_b;
      break;
    }
  }

  // update current task & ui
  timer_.start();
  current_task_ = task;
  ui->label_task->setText(TaskToString(task, false));
  ui->spinBox_solution->clear();
  ui->spinBox_solution->setPalette(input_default_palette_);
  ui->spinBox_solution->setFocus();
}

void MainWindow::CheckSolution() {
  int solution = ui->spinBox_solution->value();

  if (current_task_->result != solution) {
    current_task_->wrong_guesses.push_back(solution);
    ui->spinBox_solution->setPalette(input_error_palette_);
    ui->spinBox_solution->selectAll();
    return;
  }

  // update ui & generate new task
  current_task_->time = timer_.elapsed();
  solved_tasks_.push_back(current_task_);
  AddTaskToHistory(current_task_);
  UpdateStatistics(current_task_);
  GenerateNewCurrentTask();
}

void MainWindow::Export() {
  if (solved_tasks_.empty()) return;

  ui->pushButton_export->setEnabled(false);

  QString file_name =
      "mathe-lernen_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm") + ".txt";
  QString file_path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  QFile file(file_path + "/" + file_name);
  if (!file.open(QIODevice::ReadWrite)) {
    qDebug() << "can not create file" << file_name;
    ui->pushButton_export->setEnabled(true);
    return;
  }

  QTextStream stream(&file);

  stream << "### STATISTIK ###" << endl;
  stream << "Operation | Aufgaben | Fehlversuche | Zeit (Durchschnitt)" << endl;
  for (const OperationInfo &op : operation_infos_) {
    if (op.numTasks == 0) continue;
    stream << op.symbol.leftJustified(10, ' ') << "| "
           << QString::number(op.numTasks).leftJustified(9, ' ') << "| "
           << QString::number(op.numErrors).leftJustified(13, ' ') << "| "
           << TimeToString(op.timeTotal / op.numTasks) << endl;
  }
  stream << endl;
  stream << "### AUFGABEN ###" << endl;
  stream << "Aufgabe\t\t\t| Fehlversuche | Zeit" << endl;
  for (const std::shared_ptr<const Task> &task : solved_tasks_) {
    stream << TaskToString(task).leftJustified(24, ' ') << "| "
           << QString::number(task->wrong_guesses.size()).leftJustified(13, ' ') << "| "
           << TimeToString(task->time) << endl;
  }

  ui->pushButton_export->setEnabled(true);
}

void MainWindow::UpdateStatistics(const std::shared_ptr<const Task> &task) {
  int operation_index = operation_indices_[task->operation];
  OperationInfo &op = operation_infos_[operation_index];
  op.numTasks += 1;
  op.numErrors += task->wrong_guesses.size();
  op.timeTotal += task->time;

  ui->tableWidget_statistics->item(operation_index, 0)->setText(QString::number(op.numTasks));
  ui->tableWidget_statistics->item(operation_index, 1)->setText(QString::number(op.numErrors));
  ui->tableWidget_statistics->item(operation_index, 2)
      ->setText(TimeToString(op.timeTotal / op.numTasks));
}

void MainWindow::AddTaskToHistory(const std::shared_ptr<const Task> &task) {
  int operation_index = operation_indices_[task->operation];
  OperationInfo &op = operation_infos_[operation_index];

  int index = ui->tableWidget_history->rowCount();

  ui->tableWidget_history->insertRow(index);
  ui->tableWidget_history->setItem(index, 0, new QTableWidgetItem(TaskToString(task)));
  ui->tableWidget_history->setItem(
      index, 1, new QTableWidgetItem(QString::number(task->wrong_guesses.size())));
  ui->tableWidget_history->setItem(index, 2, new QTableWidgetItem(TimeToString(task->time)));

  ui->tableWidget_history->scrollToBottom();
}

QString MainWindow::TaskToString(const std::shared_ptr<const Task> &task, bool with_result) {
  QString string = QString::number(task->value_a) + " " +
                   operation_infos_[operation_indices_[task->operation]].symbol + " " +
                   QString::number(task->value_b) + " = ";
  if (with_result) string += QString::number(task->result);
  return string;
}

QString MainWindow::TimeToString(qint64 time) {
  qint64 time_seconds = time / 1000;
  return QString::number(time_seconds / 60) + ":" +
         QString::number(time_seconds % 60).rightJustified(2, '0');
}

void MainWindow::on_pushButton_check_clicked() { CheckSolution(); }
void MainWindow::on_pushButton_export_clicked() { Export(); }
