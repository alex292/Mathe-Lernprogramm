#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QElapsedTimer>
#include <QMainWindow>

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace Ui {
class MainWindow;
}

enum class Operation { Add, Subtract, Multiply, Divide };

struct EnumClassHash {
  template <typename T>
  std::size_t operator()(T t) const {
    return static_cast<size_t>(t);
  }
};

struct OperationInfo {
  Operation operation;
  QString symbol;
  int numTasks = 0;
  int numErrors = 0;
  qint64 timeTotal = 0;

  OperationInfo() {}
  OperationInfo(Operation _operation, const QString &_symbol)
      : operation(_operation), symbol(_symbol) {}
};

struct Task {
  int value_a;
  int value_b;
  int result;
  Operation operation;
  std::vector<int> wrong_guesses;
  qint64 time;
};

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

 private slots:
  void on_pushButton_check_clicked();

  void on_pushButton_export_clicked();

 private:
  Ui::MainWindow *ui;

  std::vector<OperationInfo> operation_infos_;
  std::unordered_map<Operation, int, EnumClassHash> operation_indices_;

  std::random_device random_device_;
  std::mt19937 random_engine_;
  std::uniform_int_distribution<int> multiplication_distribution_;
  std::uniform_int_distribution<int> add_distribution_;

  std::shared_ptr<Task> current_task_;
  std::vector<std::shared_ptr<Task>> solved_tasks_;

  QElapsedTimer timer_;

  QPalette input_default_palette_;
  QPalette input_error_palette_;

  void GenerateNewCurrentTask();
  void CheckSolution();
  void Export();
  void UpdateStatistics(const std::shared_ptr<const Task> &task);
  void AddTaskToHistory(const std::shared_ptr<const Task> &task);

  QString TaskToString(const std::shared_ptr<const Task> &task, bool with_result = true);
  QString TimeToString(qint64 time);
};

#endif  // MAINWINDOW_H
