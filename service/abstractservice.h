#ifndef ABSTRACTSERVICE_H
#define ABSTRACTSERVICE_H

#include "db/accountrepository.h"
#include "entities/account.h"
#include "qmainwindow.h"
#include "qtrpt.h"
#include <QObject>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QToolTip>
#include <QDialog>
#include <QVBoxLayout>
#include <map>
#include "auth/usersession.h"

class AbstractService : public QObject
{
    Q_OBJECT
private:
    const std::string m_filePath = ":/reports/reportsFiles/";
    std::string m_fileName = "";
protected:
    UserSession* m_session = nullptr;
    std::shared_ptr<AccountRepository> m_account_repo = nullptr;
    // pointer by reference to modify the pointer itself
    bool buildReport(QMainWindow* window, QtRPT*& report) const;

    template<class T>
    bool isPresent(const int id, std::function<std::vector<std::shared_ptr<Entity>>()> getAll) const{
        std::vector<std::shared_ptr<Entity>> all = getAll();
        // check id
        bool isPresent = false;
        for(const auto& ent : all){
            if(T* obj = dynamic_cast<T*>(ent.get()); obj != nullptr){
                if(obj->getId() == id){
                    isPresent = true;
                    break;
                }
            }
        }
        return isPresent;
    }
    int getCurYear() const;

    // charts
    void setChartProperties(QChart* chart, const QString& title) const;
    QChartView* setChartAndAxisProperties(QChart* chart, QAbstractSeries* series, const QString& title, const QStringList& categories) const;
    QChartView* createBarChart(QBarSet* barSet, const QString& title, const QStringList& categories, std::function<QString(int)> toolTipText) const;
    void setBarSetProperties(QBarSet* barSet, std::function<QString(int)> toolTipText) const;
    void createChartBox(QChartView* chartView, const int w, const int h) const;
public:
    explicit AbstractService(QObject *parent = nullptr);
    virtual ~AbstractService();
    std::string getFileName() const; // for the report
    void setFileName(const std::string name); // for the report
    // pass the function pointer to put the data into the columns of the report
    void getReport(QMainWindow* window, std::function<void(QtRPT* report)> putData, size_t recordCount) const;

signals:
};

#endif // ABSTRACTSERVICE_H
