/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                               *
 * Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                          *
 *                                                                               *
 * This file is part of libnymea-networkmanager.                                 *
 *                                                                               *
 * libnymea-networkmanager is free software: you can redistribute it and/or      *
 * modify it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License,                *
 * or (at your option) any later version.                                        *
 *                                                                               *
 * libnymea-networkmanager is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  *
 * GNU General Public License for more details.                                  *
 *                                                                               *
 * You should have received a copy of the GNU General Public License along       *
 * with libnymea-networkmanager. If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "networksettings.h"
#include "networkmanagerutils.h"

#include <QDebug>

/*! Constructs a new \l{NetworkSettings} object with the given \a parent. */
NetworkSettings::NetworkSettings(QObject *parent) :
    QObject(parent)
{
    m_settingsInterface = new QDBusInterface(NetworkManagerUtils::networkManagerServiceString(), NetworkManagerUtils::settingsPathString(), NetworkManagerUtils::settingsInterfaceString(), QDBusConnection::systemBus(), this);
    if(!m_settingsInterface->isValid()) {
        qCWarning(dcNetworkManager()) << "Invalid DBus network settings interface";
        return;
    }

    loadConnections();

    QDBusConnection::systemBus().connect(NetworkManagerUtils::networkManagerServiceString(), NetworkManagerUtils::settingsPathString(), NetworkManagerUtils::settingsInterfaceString(), "NewConnection", this, SLOT(connectionAdded(QDBusObjectPath)));
    QDBusConnection::systemBus().connect(NetworkManagerUtils::networkManagerServiceString(), NetworkManagerUtils::settingsPathString(), NetworkManagerUtils::settingsInterfaceString(), "ConnectionRemoved", this, SLOT(connectionRemoved(QDBusObjectPath)));
    QDBusConnection::systemBus().connect(NetworkManagerUtils::networkManagerServiceString(), NetworkManagerUtils::settingsPathString(), NetworkManagerUtils::settingsInterfaceString(), "PropertiesChanged", this, SLOT(propertiesChanged(QVariantMap)));
}

/*! Add the given \a settings to this \l{NetworkSettings}. Returns the dbus object path from the new settings. */
QDBusObjectPath NetworkSettings::addConnection(const ConnectionSettings &settings)
{
    QDBusMessage query = m_settingsInterface->call("AddConnection", QVariant::fromValue(settings));
    if(query.type() != QDBusMessage::ReplyMessage) {
        qCWarning(dcNetworkManager()) << query.errorName() << query.errorMessage();
        return QDBusObjectPath();
    }

    if (query.arguments().isEmpty())
        return QDBusObjectPath();

    return query.arguments().at(0).value<QDBusObjectPath>();
}

/*! Returns the list of current \l{NetworkConnection}{NetworkConnections} from this \l{NetworkSettings}. */
QList<NetworkConnection *> NetworkSettings::connections() const
{
    return m_connections.values();
}

void NetworkSettings::loadConnections()
{
    QDBusMessage query = m_settingsInterface->call("ListConnections");
    if(query.type() != QDBusMessage::ReplyMessage) {
        qCWarning(dcNetworkManager()) << query.errorName() << query.errorMessage();
        return;
    }

    if (query.arguments().isEmpty())
        return;

    const QDBusArgument &argument = query.arguments().at(0).value<QDBusArgument>();
    argument.beginArray();
    while(!argument.atEnd()) {
        QDBusObjectPath objectPath = qdbus_cast<QDBusObjectPath>(argument);
        connectionAdded(objectPath);
    }
    argument.endArray();

}

void NetworkSettings::connectionAdded(const QDBusObjectPath &objectPath)
{
    NetworkConnection *connection = new NetworkConnection(objectPath, this);
    m_connections.insert(objectPath, connection);

    qCDebug(dcNetworkManager()) << "Settings: [+]" << connection;
}

void NetworkSettings::connectionRemoved(const QDBusObjectPath &objectPath)
{
    NetworkConnection *connection = m_connections.take(objectPath);
    qCDebug(dcNetworkManager()) << "Settings: [-]" << connection;
    connection->deleteLater();
}

void NetworkSettings::propertiesChanged(const QVariantMap &properties)
{
    Q_UNUSED(properties);
    // TODO: handle settings changes
    //qCDebug(dcNetworkManager()) << "Settins: properties changed" << properties;
}
