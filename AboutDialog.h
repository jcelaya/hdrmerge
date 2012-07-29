#include <QDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>

class AboutDialog : public QDialog {
public:
    explicit AboutDialog(QWidget * parent = 0, Qt::WindowFlags f = 0) : QDialog(parent, f) {
        QHBoxLayout * layout = new QHBoxLayout(this);
        logoLabel = new QLabel(this);
        logoLabel->setPixmap(QPixmap(":/images/logo.png"));
        layout->addWidget(logoLabel);
        layout->addSpacing(12);
        text = new QLabel(tr("<h1>HDRMerge</h1><p>A software for the fussion of multiple exposures into a single high dynamic range image.</p>"
            "<p>Copyright &copy; 2012 Javier Celaya (jcelaya@gmail.com)</p>"
            "<p>This is free software: you can redistribute it and/or modify it under the terms of the GNU "
            "General Public License as published by the Free Software Foundation, either version 3 of the License, "
            "or (at your option) any later version.</p>"), this);
        text->setWordWrap(true);
        layout->addWidget(text);
        layout->setAlignment(text, Qt::AlignTop);
        setWindowTitle(tr("About HDRMerge..."));
    }
    
    /// Triggered when the dialog is closed
    void closeEvent(QCloseEvent * event) { accept(); }
    
private:
    Q_OBJECT
    
    QLabel * logoLabel;
    QLabel * text;
};
