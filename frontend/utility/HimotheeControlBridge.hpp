#pragma once

#include <memory>
#include <QString>

class OBSBasic;
class QTcpServer;

// Stage 10.6.2: localhost-only JSON HTTP bridge for external controllers.
// All callbacks run in the Qt GUI thread, as required by ActionRegistry.
class HimotheeControlBridge {
public:
 explicit HimotheeControlBridge(OBSBasic *main);
 ~HimotheeControlBridge();
 bool IsListening() const;
 quint16 Port() const;
 QString Error() const;
private:
 OBSBasic *main = nullptr;
 std::unique_ptr<QTcpServer> server;
 QString error;
 void Accept();
};
