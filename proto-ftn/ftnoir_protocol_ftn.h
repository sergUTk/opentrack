/* Homepage         http://facetracknoir.sourceforge.net/home/default.htm        *
 *                                                                               *
 * ISC License (ISC)                                                             *
 *                                                                               *
 * Copyright (c) 2015, Wim Vriend                                                *
 *                                                                               *
 * Permission to use, copy, modify, and/or distribute this software for any      *
 * purpose with or without fee is hereby granted, provided that the above        *
 * copyright notice and this permission notice appear in all copies.             *
 */
#pragma once

#include "ui_ftnoir_ftncontrols.h"
#include <QThread>
#include <QUdpSocket>
#include <QMessageBox>
#include <cmath>
#include "opentrack/plugin-api.hpp"
#include "opentrack-compat/options.hpp"
using namespace options;

struct settings : opts {
    value<int> ip1, ip2, ip3, ip4, port;
    settings() :
        opts("udp-proto"),
        ip1(b, "ip1", 192),
        ip2(b, "ip2", 168),
        ip3(b, "ip3", 0),
        ip4(b, "ip4", 2),
        port(b, "port", 4242)
    {}
};

class FTNoIR_Protocol : public IProtocol
{
public:
    FTNoIR_Protocol();
    bool correct();
    void pose(const double *headpose);
    QString game_name() {
        return "UDP Tracker";
    }
private:
    QUdpSocket outSocket;
    settings s;
};

// Widget that has controls for FTNoIR protocol client-settings.
class FTNControls: public IProtocolDialog
{
    Q_OBJECT
public:
    FTNControls();
    void register_protocol(IProtocol *) {}
    void unregister_protocol() {}
private:
    Ui::UICFTNControls ui;
    settings s;
private slots:
    void doOK();
    void doCancel();
};

class FTNoIR_ProtocolDll : public Metadata
{
public:
    QString name() { return QString("UDP receiver"); }
    QIcon icon() { return QIcon(":/images/facetracknoir.png"); }
};
