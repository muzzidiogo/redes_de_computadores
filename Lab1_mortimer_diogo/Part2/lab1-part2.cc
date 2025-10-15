#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0

// New Network Topology
//
//       10.1.1.0                         10.1.3.0
// n0 -------------- n1   n2   n3   n4 -------------- n5
//    point-to-point |    |    |    |  point-to-point
//                   ================
//                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part2");

int main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nCsma = 3;
    uint32_t nPackets = 1;
    Time stopTime = Seconds(25.0); 

    CommandLine cmd (__FILE__);
    cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue ("nPackets", "Number of packets to send (max 20)", nPackets);
    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
    cmd.Parse (argc,argv);

    if (verbose)
    {
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    if (nPackets > 20)
    {
        NS_LOG_WARN ("nPackets > 20; setting nPackets to 20");
        nPackets = 20;
    }

    nCsma = nCsma == 0 ? 1 : nCsma;

    NodeContainer p2pNodes;
    p2pNodes.Create (2);

    NodeContainer csmaNodes;
    csmaNodes.Add (p2pNodes.Get (1));
    csmaNodes.Create (nCsma);

    NodeContainer p2pNodes2;
    p2pNodes2.Add (csmaNodes.Get (nCsma)); 
    p2pNodes2.Create (1);                 

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install (p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install (csmaNodes);

    NetDeviceContainer p2pDevices2;
    p2pDevices2 = pointToPoint.Install (p2pNodes2);


    InternetStackHelper stack;
    stack.Install (p2pNodes.Get (0)); 
    stack.Install (csmaNodes);        
    stack.Install (p2pNodes2.Get (1)); 

    Ipv4AddressHelper address;

    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign (p2pDevices);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign (csmaDevices);

    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces2;
    p2pInterfaces2 = address.Assign (p2pDevices2);

    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (p2pNodes2.Get (1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (stopTime);

    UdpEchoClientHelper echoClient (p2pInterfaces2.GetAddress (1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (stopTime);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    pointToPoint.EnablePcapAll ("lab1-part2");
    csma.EnablePcap ("lab1-part2", csmaDevices.Get (1), true);

    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}