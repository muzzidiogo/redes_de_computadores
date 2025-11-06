/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * ns-3 simulation script for Part 2: RTT Fairness Comparison (CUBIC vs NewReno)
 *
 * Topology: 
 * N1 (Source) -- [100Mbps, 0.01ms] -- N2 -- [Bottleneck] -- N3 
 * |
 * / \
 * /   \
 * /     \
 * [100Mbps, 0.01ms] -> N4 (Dest 1: Short RTT)   [100Mbps, 50ms] -> N5 (Dest 2: Long RTT)
 *
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpRttFairnessComparison");

const double SIMULATION_DURATION = 20.0;
const double FLOW_START_TIME = 1.0;
const double SINK_START_TIME = 0.0;
const std::string COMMON_DATA_RATE = "100Mbps";
const std::string COMMON_DELAY = "0.01ms";
const std::string LONG_DELAY = "50ms"; 

static std::map<uint32_t, bool> firstCwnd;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, uint32_t> cWndValue;

static uint32_t
GetSocketIndexFromContext (std::string context)
{
  std::size_t const n1 = context.find ("/SocketList/");
  if (n1 == std::string::npos)
    {
      return 0;
    }
  std::size_t const n2 = context.find_first_of ("/", n1 + 12);
  return std::stoul (context.substr (n1 + 12, n2 - n1 - 12));
}

static void
CwndTracer (std::string context, uint32_t oldval, uint32_t newval)
{
  uint32_t flowId = GetSocketIndexFromContext (context);

  if (firstCwnd.find(flowId) == firstCwnd.end()) 
    {
      firstCwnd[flowId] = true;
    }

  if (firstCwnd[flowId])
    {
      *cWndStream[flowId]->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd[flowId] = false;
    }
  
  *cWndStream[flowId]->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue[flowId] = newval;
}

static void
TraceCwnd (std::string cwnd_tr_file_name, uint32_t nodeId, uint32_t socketIndex)
{
  AsciiTraceHelper ascii;
  cWndStream[socketIndex] = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  
  std::stringstream path;
  path << "/NodeList/" << nodeId << "/$ns3::TcpL4Protocol/SocketList/" 
       << socketIndex << "/CongestionWindow";
       
  Config::Connect (path.str (), MakeCallback (&CwndTracer));
}

int main (int argc, char *argv[])
{
  // --- 1. Command Line Arguments and Defaults ---
  std::string transport_prot = "TcpCubic";
  std::string bottleneck_data_rate = "1Mbps";
  std::string bottleneck_delay = "20ms";
  double errorRate = 0.00001;
  uint32_t nFlows = 2; 
  uint32_t runIndex = 0; 
  uint64_t data_mbytes = 0;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("transport_prot", "Transport protocol: TcpCubic or TcpNewReno", transport_prot);
  cmd.AddValue ("dataRate", "Bottleneck link data rate", bottleneck_data_rate);
  cmd.AddValue ("delay", "Bottleneck link delay", bottleneck_delay);
  cmd.AddValue ("errorRate", "Bottleneck link byte error rate", errorRate);
  cmd.AddValue ("nFlows", "Total number of flows (must be even, max 20)", nFlows);
  cmd.AddValue ("run", "Run index for setting repeatable seeds (0-9)", runIndex); 
  cmd.Parse (argc, argv);

  if (nFlows == 0 || nFlows % 2 != 0 || nFlows > 20)
    {
      NS_FATAL_ERROR ("nFlows must be an even number between 2 and 20.");
    }

  if (transport_prot != "TcpCubic" && transport_prot != "TcpNewReno")
    {
      NS_FATAL_ERROR ("transport_prot must be either TcpCubic or TcpNewReno.");
    }

  std::string full_transport_prot = std::string ("ns3::") + transport_prot;
  
  SeedManager::SetSeed (1);
  SeedManager::SetRun (runIndex);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (full_transport_prot)));
  
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (536));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));

  // --- 2. Topology Creation (5 Nodes) ---
  NodeContainer nodes;
  nodes.Create (5); 
  Ptr<Node> n1 = nodes.Get (0);
  Ptr<Node> n2 = nodes.Get (1);
  Ptr<Node> n3 = nodes.Get (2);
  Ptr<Node> n4 = nodes.Get (3);
  Ptr<Node> n5 = nodes.Get (4);

  PointToPointHelper highSpeedLink;
  highSpeedLink.SetDeviceAttribute ("DataRate", StringValue (COMMON_DATA_RATE));
  highSpeedLink.SetChannelAttribute ("Delay", StringValue (COMMON_DELAY)); // 0.01ms

  PointToPointHelper longDelayLink;
  longDelayLink.SetDeviceAttribute ("DataRate", StringValue (COMMON_DATA_RATE));
  longDelayLink.SetChannelAttribute ("Delay", StringValue (LONG_DELAY)); // 50ms

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneck_data_rate));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneck_delay));

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
  bottleneckLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (em));

  NetDeviceContainer d1d2 = highSpeedLink.Install (n1, n2); 
  NetDeviceContainer d2d3 = bottleneckLink.Install (n2, n3);
  NetDeviceContainer d3d4 = highSpeedLink.Install (n3, n4); 
  NetDeviceContainer d3d5 = longDelayLink.Install (n3, n5); 

  InternetStackHelper stack;
  stack.Install (nodes);

  // --- 3. IP Addressing and Routing (3 Networks) ---
  Ipv4AddressHelper address;
  
  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (d1d2);

  address.NewNetwork ();
  address.SetBase ("10.2.2.0", "255.255.255.0");
  address.Assign (d2d3);

  address.NewNetwork ();
  address.SetBase ("10.3.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = address.Assign (d3d4);

  address.NewNetwork ();
  address.SetBase ("10.4.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i5 = address.Assign (d3d5);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  // --- 4. Application Setup (Heterogeneous Flows) ---
  uint16_t port = 50000;
  uint32_t numDest1Flows = nFlows / 2;
  uint32_t numDest2Flows = nFlows / 2;
  
  Ipv4Address sink1IpAddress = i3i4.GetAddress (1); 
  Ipv4Address sink2IpAddress = i3i5.GetAddress (1); 

  ApplicationContainer sourceApps;
  ApplicationContainer sinkApps;
  
  for (uint32_t i = 0; i < nFlows; ++i)
    {
      Ptr<Node> sinkNode;
      Ipv4Address sinkIp;
      uint16_t currentPort = port + i;

      if (i < numDest1Flows)
        {
          sinkNode = n4;
          sinkIp = sink1IpAddress;
        }
      else
        {
          sinkNode = n5;
          sinkIp = sink2IpAddress;
        }

      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), currentPort));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      sinkApps.Add (sinkHelper.Install (sinkNode));
      
      AddressValue remoteAddress (InetSocketAddress (sinkIp, currentPort));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (536));
      ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));

      sourceApps.Add (ftp.Install (n1));
    }
    
  sinkApps.Start (Seconds (SINK_START_TIME));
  sinkApps.Stop (Seconds (SIMULATION_DURATION));

  sourceApps.Start (Seconds (FLOW_START_TIME));
  sourceApps.Stop (Seconds (SIMULATION_DURATION - 1));

  // --- 5. Tracing and Flow Monitor ---
  double traceStartTime = FLOW_START_TIME + 0.00001; 
  for (uint32_t i = 0; i < nFlows; ++i)
    {
      std::stringstream flowIdStr;
      flowIdStr << "flow-" << i;
      std::string cwndFileName = "cwnd-trace-" + flowIdStr.str() + ".csv";
      Simulator::Schedule (Seconds (traceStartTime), &TraceCwnd, cwndFileName, 0, i);
    }

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll ();

  // --- 6. Execution and Data Extraction ---
  Simulator::Stop (Seconds (SIMULATION_DURATION));
  Simulator::Run ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

  double dest1Goodput = 0.0; 
  double dest2Goodput = 0.0; 
  
  for (auto const& iter : stats)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter.first);
      
      if (t.destinationAddress == Ipv4Address("10.3.3.2"))
        {
          dest1Goodput += (double)iter.second.rxBytes * 8 / (SIMULATION_DURATION - FLOW_START_TIME) / 1000000.0;
        }
      else if (t.destinationAddress == Ipv4Address("10.4.4.2"))
        {
          dest2Goodput += (double)iter.second.rxBytes * 8 / (SIMULATION_DURATION - FLOW_START_TIME) / 1000000.0;
        }
    }

  double avgDest1Goodput = dest1Goodput / numDest1Flows;
  double avgDest2Goodput = dest2Goodput / numDest2Flows;

  std::cout << "--- RTT Fairness Results ---\n";
  std::cout << "Protocol: " << transport_prot << "\n";
  std::cout << "NFlows: " << nFlows << "\n";
  std::cout << "RunIndex: " << runIndex << "\n";
  std::cout << "Average Goodput (Dest 1 - Short RTT): " << avgDest1Goodput << " Mbps\n";
  std::cout << "Average Goodput (Dest 2 - Long RTT): " << avgDest2Goodput << " Mbps\n";
  std::cout << "----------------------------\n";

  Simulator::Destroy ();
  return 0;
}