#include "abstractservice.h"
#include "entities/account.h"

AbstractService::AbstractService(QObject *parent)
    : QObject{parent}
{
    m_account_repo = std::make_shared<AccountRepository>(AccountRepository());
}
AbstractService::~AbstractService(){}


std::string AbstractService::getFileName() const{
    return m_fileName;
}
void AbstractService::setFileName(const std::string name){
    const std::string ending = "xml";
    if(name.length() < ending.length() + 2){ // 1 for . and at least 1 for the name
        throw std::invalid_argument("The file name is invalid!");
    }
    // check the ending
    if(ending == name.substr(name.length()-ending.length(), ending.length())){
        m_fileName = name;
        qDebug() << "The new name for the file has been set";
    }else{
        throw std::invalid_argument("The file name is invalid!");
    }
}
bool AbstractService::buildReport(QMainWindow* window, QtRPT*& report) const{
    if(m_fileName == "")
        return false;
    if(report){
        delete report;
        report = nullptr;
    }
    const std::string file = m_filePath + m_fileName;
    report = new QtRPT(window);
    report->loadReport(file.c_str());
    return true;
}

void AbstractService::getReport(QMainWindow* window,
                                std::function<void(QtRPT* report)> putData, size_t recordCount) const{
    QtRPT* report = nullptr;
    if(!buildReport(window, report)){
        return;
    }
    if(report){ // if we are here, it won't be nullptr anyway, so we can get rid of this if
        connect(report, &QtRPT::setDSInfo, [=](DataSetInfo& dsinfo){
            dsinfo.recordCount = recordCount;
        });
        connect(report, &QtRPT::setValueImage, [=](const int recno, const QString paramname,
                                              QImage& paramValue, const int reportpage){
            Q_UNUSED(reportpage);
            Q_UNUSED(recno);
            if(paramname == "image"){
                std::shared_ptr<QImage> img(new QImage(":/img/img/dollar.png"));
                paramValue = *img;
            }

        });
        putData(report);
        report->printExec();
        delete report;
        report = nullptr;
    }
}
int AbstractService::getCurYear() const{
    std::chrono::year_month_day today =
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    return static_cast<int>(today.year());
}
QChartView* AbstractService::createChart(const QVector<QPointF>& points) const{
    // TODO: set some properties
    QLineSeries *series = new QLineSeries();
    series->append(points);
    QChart* chart = new QChart();
    chart->addSeries(series);
    QChartView* chartView = new QChartView(chart);
    return chartView;
}
void AbstractService::createChartBox(QChartView* chartView, const int w, const int h) const{
    // TODO: set some properties
    QDialog dialog;
    QVBoxLayout layout(&dialog);
    layout.addWidget(chartView);
    dialog.resize(w, h);
    dialog.exec();
}
