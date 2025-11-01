/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * “TCP Westwood(+) Protocol Implementation in ns-3”
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

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
// New required include for flow monitor in recent ns-3 versions
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBottleneckComparison");

// Maps key by FLOW INDEX
static std::map<uint32_t, bool> firstCwnd;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, uint32_t> cWndValue;

static uint32_t
GetNodeIdFromContext (std::string context)
{
  std::size_t const n1 = context.find_first_of ("/", 1);
  std::size_t const n2 = context.find_first_of ("/", n1 + 1);
  return std::stoul (context.substr (n1 + 1, n2 - n1 - 1));
}

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

// Global constants
const double SIMULATION_DURATION = 20.0;
const double FLOW_START_TIME = 1.0;
const double SINK_START_TIME = 0.0;
const std::string COMMON_DATA_RATE = "100Mbps";
const std::string COMMON_DELAY = "0.01ms";


int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpCubic";
  std::string bottleneck_data_rate = "1Mbps";
  std::string bottleneck_delay = "20ms";
  double errorRate = 0.00001;
  uint32_t nFlows = 1;
  uint64_t data_mbytes = 0;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpCubic or TcpNewReno", transport_prot);
  cmd.AddValue ("dataRate", "Bottleneck link data rate (e.g., 1Mbps)", bottleneck_data_rate);
  cmd.AddValue ("delay", "Bottleneck link delay (e.g., 20ms)", bottleneck_delay);
  cmd.AddValue ("errorRate", "Bottleneck link byte error rate (e.g., 0.00001)", errorRate);
  cmd.AddValue ("nFlows", "Number of concurrent TCP flows (max 20)", nFlows);
  cmd.Parse (argc, argv);

  if (nFlows == 0 || nFlows > 20)
    {
      NS_FATAL_ERROR ("nFlows must be between 1 and 20.");
    }

  if (transport_prot != "TcpCubic" && transport_prot != "TcpNewReno")
    {
      NS_FATAL_ERROR ("transport_prot must be either TcpCubic or TcpNewReno.");
    }

  std::string full_transport_prot = std::string ("ns3::") + transport_prot;
  
  SeedManager::SetSeed (1);
  SeedManager::SetRun (1);

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 
                      TypeIdValue (TypeId::LookupByName (full_transport_prot)));
  
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (536));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));

  NodeContainer nodes;
  nodes.Create (4);
  Ptr<Node> n1 = nodes.Get (0);
  Ptr<Node> n2 = nodes.Get (1);
  Ptr<Node> n3 = nodes.Get (2);
  Ptr<Node> n4 = nodes.Get (3);

  PointToPointHelper highSpeedLink;
  highSpeedLink.SetDeviceAttribute ("DataRate", StringValue (COMMON_DATA_RATE));
  highSpeedLink.SetChannelAttribute ("Delay", StringValue (COMMON_DELAY));

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneck_data_rate));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneck_delay));

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (errorRate));
  bottleneckLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (em));

  NetDeviceContainer d1d2 = highSpeedLink.Install (n1, n2);

  NetDeviceContainer d2d3 = bottleneckLink.Install (n2, n3);

  NetDeviceContainer d3d4 = highSpeedLink.Install (n3, n4);
  
  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = address.Assign (d1d2);

  address.NewNetwork ();
  address.SetBase ("10.2.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = address.Assign (d2d3);

  address.NewNetwork ();
  address.SetBase ("10.3.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = address.Assign (d3d4);
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  uint16_t port = 50000;
  ApplicationContainer sourceApps;
  ApplicationContainer sinkApps;
  
  Ipv4Address sinkIpAddress = i3i4.GetAddress (1);

  for (uint32_t i = 0; i < nFlows; ++i)
    {
      Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port + i));
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
      
      sinkApps.Add (sinkHelper.Install (n4));
      
      AddressValue remoteAddress (InetSocketAddress (sinkIpAddress, port + i));
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

  // --- FIX APPLIED HERE: Schedule TraceCwnd slightly after FLOW_START_TIME ---
  double traceStartTime = FLOW_START_TIME + 0.00001; 
  for (uint32_t i = 0; i < nFlows; ++i)
    {
      std::stringstream flowIdStr;
      flowIdStr << "flow-" << i;

      std::string cwndFileName = "cwnd-trace-" + flowIdStr.str() + ".csv";
      
      // The original script called TraceCwnd directly. Now we schedule it.
      Simulator::Schedule (Seconds (traceStartTime), &TraceCwnd, cwndFileName, 0, i);
    }
  // --- END FIX ---


  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll ();

  NS_LOG_INFO ("Running simulation for " << SIMULATION_DURATION << " seconds.");
  Simulator::Stop (Seconds (SIMULATION_DURATION));
  Simulator::Run ();

  std::cout << "\n======================================================\n";
  std::cout << "Flow Monitor Results (" << transport_prot << ") - Goodput\n";
  std::cout << "======================================================\n";

  // 1. Get the Flow Stats Container
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowHelper.GetClassifier ());
  // Change GetStats() to GetFlowStats() to match the user's specific ns-3 environment
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats ();

  double totalGoodput = 0.0;
  
  for (auto const& iter : stats)
    {
      // 2. Use GetFlowClassifier to retrieve the 5-tuple
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter.first);
      
      double goodput = (double)iter.second.rxBytes * 8 / (SIMULATION_DURATION - FLOW_START_TIME) / 1000000.0;
      totalGoodput += goodput;

      // 3. Print the flow details
      std::cout << "Flow ID " << iter.first << " (" << t.sourceAddress << " -> " << t.destinationAddress 
                << ", Port " << t.destinationPort << "): "
                << goodput << " Mbps (Goodput)\n";
    }

  std::cout << "\nTotal Aggregate Goodput: " << totalGoodput << " Mbps\n";
  std::cout << "======================================================\n";

  Simulator::Destroy ();
  return 0;
}
