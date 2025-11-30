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
/*****************************************
                CHARTS
 *****************************************/
void AbstractService::setChartProperties(QChart* chart, const QString& title) const{
    // 1. Theme: The "Easy Button" for good looks
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    // 2. Title & Legend
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI", 16, QFont::Bold));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setBackgroundVisible(true);
}
void AbstractService::setBarSetProperties(QBarSet* barSet, std::function<QString(int)> toolTipText) const{
    // Remove the jagged outline (Pen) to make bars look smoother
    barSet->setPen(Qt::NoPen);
    // specific color that contrasts well with the dark blue theme
    barSet->setColor(QColor(0, 180, 255));
    // Labels: Show the specific value on top of the bar
    barSet->setLabelColor(Qt::white);
    barSet->setLabelFont(QFont("Arial", 10, QFont::Bold));

    // Connect the "hovered" signal
    // 'status' is true when entering the bar, false when leaving
    // 'index' tells WHICH bar is being hovered (0, 1, 2...)
    QObject::connect(barSet, &QBarSet::hovered, [=](bool status, int index) {
        if (status) {
            // Show the standard Qt Tooltip at the mouse cursor position
            QToolTip::showText(QCursor::pos(), toolTipText(index));
        } else {
            // Hide the tooltip when the mouse leaves the bar
            QToolTip::hideText();
        }
    });
}
QChartView* AbstractService::createBarChart(QBarSet* barSet, const QString& title, const QStringList& categories, std::function<QString(int)> toolTipText) const{
    setBarSetProperties(barSet, toolTipText);
    QBarSeries *series = new QBarSeries();
    series->append(barSet);
    QChart* chart = new QChart();
    setChartProperties(chart, title);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    QValueAxis* axisY = new QValueAxis();
    axisY->setGridLineColor(QColor(255, 255, 255, 50)); // White with transparency
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->addSeries(series);
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    return chartView;
}
void AbstractService::createChartBox(QChartView* chartView, const int w, const int h) const{
    QDialog dialog;
    QVBoxLayout layout(&dialog);    
    layout.addWidget(chartView);
    dialog.resize(w, h);
    dialog.exec();
}
