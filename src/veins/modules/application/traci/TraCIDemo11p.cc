#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;

        // Open log file
        logFile.open("C:/Dev/veins/examples/veins/traCIDemoLog.txt", std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            EV << "Could not open log file for writing" << std::endl;
        }

        isMalicious = par("isMalicious").boolValue();

        safeRoute = {"-39539626", "-5445204#2", "-5445204#1", "113939244#2", "-126606716",
                                 "23339459", "30405358#1", "85355912", "85355911#0", "85355911#1",
                                 "30405356", "5931612", "30350450#0", "30350450#1", "30350450#2",
                                 "4006702#0", "4006702#1", "4900043", "4900041#1"};
    }
}

TraCIDemo11p::~TraCIDemo11p() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    findHost()->getDisplayString().setTagArg("i", 1, "green");

    // Check if the message is from a malicious vehicle
    if (wsm->getSenderAddress() != myId && mobility->getRoadId()[0] != ':') {
        // Change route for all non-malicious vehicles
        for (const auto& edgeId : safeRoute) {
                        traciVehicle->changeRoute(edgeId.c_str(), 9999);
                    }

        // Log the route change
        logMessage("Changed route due to malicious broadcast: " + std::string(wsm->getDemoData()));
    }

    if (!sentMessage) {
        sentMessage = true;
        // repeat the received traffic update once in 2 seconds plus some random delay
        wsm->setSenderAddress(myId);
        wsm->setSerial(3);
        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
    }

    // Log the received message
    logMessage("Received WSM: " + std::string(wsm->getDemoData()));
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        // send this message on the service channel until the counter is 3 or higher.
        // this code only runs when channel switching is enabled
        sendDown(wsm->dup());
        wsm->setSerial(wsm->getSerial() + 1);
        if (wsm->getSerial() >= 3) {
            // stop service advertisements
            stopService();
            delete (wsm);
        }
        else {
            scheduleAt(simTime() + 1, wsm);
        }
    }
    else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);

    if (isMalicious && simTime() > 40 && !sentMessage) {
            findHost()->getDisplayString().setTagArg("i", 1, "red");
                    sentMessage = true;

                    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
                    populateWSM(wsm);
                    wsm->setDemoData(mobility->getRoadId().c_str());

                    // host is standing still due to crash
                    if (dataOnSch) {
                        startService(Channel::sch2, 42, "Traffic Information Service");
                        // started service and server advertising, schedule message to self to send later
                        scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
                    }
                    else {
                        // send right away on CCH, because channel switching is disabled
                        sendDown(wsm);
                    }

                    // Log the sent message
                    logMessage("Sent fake WSM: " + std::string(wsm->getDemoData()));

    } else {
        if (mobility->getSpeed() < 1) {
                // stopped for for at least 10s?
                if (simTime() - lastDroveAt >= 10 && sentMessage == false) {
                    findHost()->getDisplayString().setTagArg("i", 1, "red");
                    sentMessage = true;

                    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
                    populateWSM(wsm);
                    wsm->setDemoData(mobility->getRoadId().c_str());

                    // host is standing still due to crash
                    if (dataOnSch) {
                        startService(Channel::sch2, 42, "Traffic Information Service");
                        // started service and server advertising, schedule message to self to send later
                        scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
                    }
                    else {
                        // send right away on CCH, because channel switching is disabled
                        sendDown(wsm);
                    }

                    // Log the sent message
                    logMessage("Sent WSM: " + std::string(wsm->getDemoData()));
                }
            }
            else {
                lastDroveAt = simTime();
            }
    }



}

void TraCIDemo11p::logMessage(const std::string& message)
{
    if (logFile.is_open()) {
        logFile << simTime() << ": " << message << std::endl;
    }
}
