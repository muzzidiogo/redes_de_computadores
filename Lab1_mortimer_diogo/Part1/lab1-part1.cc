#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

//
//       10.1.2.0          10.1.1.0
// n2 -------------- n0 -------------- n1
//    point-to-point    point-to-point
//                   |
//                 p |
//                 2 | 10.1.3.0
//                 p |
//                   |
//                   n3

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab1Part1");

int
main (int argc, char *argv[])
{
  uint32_t nClients = 1;
  uint32_t nPackets = 1;

  CommandLine cmd;
  cmd.AddValue ("nClients", "Number of client nodes (max 5)", nClients);
  cmd.AddValue ("nPackets", "Number of packets per client (max 5)", nPackets);
  cmd.Parse (argc, argv);

  nClients = std::max<uint32_t> (1, std::min<uint32_t> (nClients, 5));
  nPackets = std::max<uint32_t> (1, std::min<uint32_t> (nPackets, 5));

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (nClients + 1);

  Ptr<Node> server = nodes.Get (0);

  InternetStackHelper stack;
  stack.Install (nodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  uint16_t port = 15;
  UdpEchoServerHelper echoServer (port);
  ApplicationContainer serverApps = echoServer.Install (server);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (20.0));

  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  rand->SetAttribute ("Min", DoubleValue (0.0));
  rand->SetAttribute ("Max", DoubleValue (1.0));

  Ipv4Address serverAddress = Ipv4Address ("10.1.1.2");

  for (uint32_t i = 1; i <= nClients; ++i)
    {
      NodeContainer pair = NodeContainer (nodes.Get (i), server);

      NetDeviceContainer devices = pointToPoint.Install (pair);

      Ipv4AddressHelper address;
      std::ostringstream base;
      base << "10.1." << i << ".0";
      address.SetBase (base.str ().c_str (), "255.255.255.0");
      Ipv4InterfaceContainer interfaces = address.Assign (devices);

      UdpEchoClientHelper echoClient (serverAddress, port);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

      ApplicationContainer clientApp = echoClient.Install (nodes.Get (i));

      double start = 2.0 + rand->GetValue () * 5.0;
      clientApp.Start (Seconds (start));
      clientApp.Stop (Seconds (20.0));
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (20.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}