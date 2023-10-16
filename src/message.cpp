#include "message.h"

#include <QApplication>
#include <QTextStream>


static
int term_prompt(const QString& title, const QString& text, QMessageBox::StandardButtons buttons)
{
    QTextStream out (stdout);
    out << QString("[%1]\n%2\n---\n").arg(title).arg(text);
    std::vector<std::pair<QString, int>> opts;
    if (buttons & QMessageBox::Ok)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "OK"), QMessageBox::Ok));
    if (buttons & QMessageBox::Yes)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Yes"), QMessageBox::Yes));
    if (buttons & QMessageBox::No)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "No"), QMessageBox::No));
    if (buttons & QMessageBox::Abort)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Abort"), QMessageBox::Abort));
    if (buttons & QMessageBox::Cancel)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Cancel"), QMessageBox::Cancel));
    if (buttons & QMessageBox::Ignore)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Ignore"), QMessageBox::Ignore));
    if (buttons & QMessageBox::Reset)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Reset"), QMessageBox::Reset));
    if (buttons & QMessageBox::RestoreDefaults)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Restore Defaults"), QMessageBox::RestoreDefaults));
    if (buttons & QMessageBox::Save)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Save"), QMessageBox::Save));
    if (buttons & QMessageBox::Discard)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Discard"), QMessageBox::Discard));
    if (buttons & QMessageBox::Apply)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Apply"), QMessageBox::Apply));
    if (buttons & QMessageBox::Close)
        opts.push_back(std::make_pair(QApplication::translate("QPlatformTheme", "Close"), QMessageBox::Close));
    if (opts.size() <= 1)
        return buttons;
    for (size_t i = 0; i < opts.size(); i++)
        out << QString("%1%2 [%3]").arg(i==0 ? "" : ", ").arg(opts[i].first).arg(i+1);
    out << ":";
    out.flush();
    QTextStream in (stdin);
    uint num = 0;
    while (true)
    {
        num = in.readLine().toUInt();
        if (num >= 1 && num <= opts.size())
            break;
        out << "Invalid option, try again: ";
        out.flush();
    }
    return opts[num-1].second;
}

static
int message(QWidget *parent, QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons)
{
    if (!parent)
        return term_prompt(title, text, buttons);

    // emulate the behaviour of QMessageBox::warning(...), but raise as active window
    QMessageBox w(parent);
    w.setWindowTitle(title);
    w.setText(text);
    w.setIconPixmap(QMessageBox::standardIcon(icon)); // setIcon() plays a sound on windows
    for (uint mask = QMessageBox::FirstButton; mask <= QMessageBox::LastButton; mask <<= 1)
        if (mask & buttons)
            w.addButton((QMessageBox::StandardButton)mask);
    parent->setWindowState(Qt::WindowActive);
    //w.setWindowState(Qt::WindowActive);
    if (w.exec() == -1)
        return QMessageBox::Cancel;
    return w.standardButton(w.clickedButton());
}

int warn(QWidget *parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons)
{
    return message(parent, QMessageBox::Warning, title, text, buttons);
}

int warn(QWidget *parent, const QString& text, QMessageBox::StandardButtons buttons)
{
    return message(parent, QMessageBox::Warning, QApplication::tr("Warning"), text, buttons);
}

int info(QWidget *parent, const QString& text, QMessageBox::StandardButtons buttons)
{
    return message(parent, QMessageBox::Information, QApplication::tr("Information"), text, buttons);
}
