#ifndef FORMSEARCHCONTROL_H
#define FORMSEARCHCONTROL_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMessageBox>
#include <QElapsedTimer>

#include <deque>

#include "searchthread.h"
#include "protobasedialog.h"
#include "settings.h"

namespace Ui {
class FormSearchControl;
}

class MainWindow;


class FormSearchControl : public QWidget
{
    Q_OBJECT

public:
    explicit FormSearchControl(MainWindow *parent);
    ~FormSearchControl();

    QVector<uint64_t> getResults();
    SearchConfig getSearchConfig();
    bool setSearchConfig(SearchConfig s, bool quiet);

    void stopSearch();
    bool setList64(QString path, bool quiet);

    void searchLockUi(bool lock);

    void setSearchMode(int mode);

signals:
    void selectedSeedChanged(uint64_t seed);
    void searchStatusChanged(bool running);
    void resultsAdded(int cnt);

public slots:
    int warning(QString text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);
    void openProtobaseMsg(QString path);
    void closeProtobaseMsg();

    void on_buttonClear_clicked();
    void on_buttonStart_clicked();
    void on_buttonMore_clicked();

    void on_listResults_itemSelectionChanged();
    void on_listResults_customContextMenuRequested(const QPoint& pos);

    void on_buttonSearchHelp_clicked();

    void on_comboSearchType_currentIndexChanged(int index);

    void pasteResults();
    int pasteList(bool dummy);
    int searchResultsAdd(QVector<uint64_t> seeds, bool countonly);
    void searchProgressReset();
    void searchProgress(uint64_t last, uint64_t end, int64_t seed);
    void searchFinish(bool done);
    void resultTimeout();
    void removeCurrent();
    void copyResults();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

public: 
    struct TProg { uint64_t ns, prog; };
private:
    MainWindow *parent;
    Ui::FormSearchControl *ui;
    ProtoBaseDialog *protodialog;
    SearchMaster sthread;
    QTimer stimer;
    QElapsedTimer elapsed;
    std::deque<TProg> proghist;

    // the seed list option is not stored in a widget but is loaded with the "..." button
    QString slist64path;
    QString slist64fnam; // file name without directory
    std::vector<uint64_t> slist64;

    // buffer for seed candidates while search is running
    std::vector<uint64_t> slist;

    // min and max seeds values
    uint64_t smin, smax;
};

#endif // FORMSEARCHCONTROL_H
