/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QNetworkReply;

namespace Ui
{
    class SettingsDialog;
    class ProxyDialog;
}

class ProxyDialog : public QDialog
{
Q_OBJECT

public:
    explicit ProxyDialog( QWidget* parent = 0 );
    ~ProxyDialog() {};

    void saveSettings();

private:
    Ui::ProxyDialog* ui;
};

class SettingsDialog : public QDialog
{
Q_OBJECT

public:
    explicit SettingsDialog( QWidget* parent = 0 );
    ~SettingsDialog();

signals:
    void settingsChanged();
    
protected:
    void changeEvent( QEvent* e );

private slots:
    void onRejected();

    void showPathSelector();

    void toggleUpnp( bool preferStaticEnabled );
    void showProxySettings();

    void testLastFmLogin();
    void onLastFmFinished();

    void addScriptResolver();
    void scriptSelectionChanged();
    void removeScriptResolver();
    
private:
    Ui::SettingsDialog* ui;

    ProxyDialog m_proxySettings;
    bool m_rejected;
    QNetworkReply* m_testLastFmQuery;
};

#endif // SETTINGSDIALOG_H
