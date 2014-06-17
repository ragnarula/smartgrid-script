#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main (){
	LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
	LogComponentEnable("PacketSink",LOG_LEVEL_INFO);
	//Wifi Nodes Setup	
	NodeContainer wifiApplianceNodes;
	wifiApplianceNodes.Create (10);
	Ptr<Node> wifiApNode = CreateObject<Node>();
	Ptr<Node> espNode = CreateObject<Node>();	
	Ptr<Node> mobileWifiNode = CreateObject<Node>();

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	WifiHelper wifi = WifiHelper::Default ();
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

	Ssid ssid = Ssid ("home-area-network");
	mac.SetType ("ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ActiveProbing", BooleanValue (false));
	//device container with all client devices
	//AP not included
	NetDeviceContainer wifiDevices;
	wifiDevices = wifi.Install (phy, mac, wifiApplianceNodes);
	wifiDevices.Add(wifi.Install(phy,mac,espNode));
	wifiDevices.Add(wifi.Install(phy,mac,mobileWifiNode));

	mac.SetType ("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid));
	
	NetDeviceContainer apDevices;
	apDevices = wifi.Install (phy, mac, wifiApNode);

	MobilityHelper mobility;

	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
			"MinX", DoubleValue (0.0),
			"MinY", DoubleValue (0.0),
			"DeltaX", DoubleValue (5.0),
			"DeltaY", DoubleValue (5.0),
			"GridWidth", UintegerValue (4),
			"LayoutType", StringValue ("RowFirst"));

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
			"Bounds", RectangleValue (Rectangle (-50, 100, -50,100)));
	//Random walk model for mobile node
	mobility.Install (mobileWifiNode);
	//Constant position for all others.
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApplianceNodes);
	mobility.Install (wifiApNode);
	mobility.Install (espNode);


	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
			"MinX", DoubleValue (0.0),
			"MinY", DoubleValue (25.0),
			"DeltaX", DoubleValue (5.0),
			"DeltaY", DoubleValue (5.0),
			"GridWidth", UintegerValue (4),
			"LayoutType", StringValue ("RowFirst"));

	//Internet p2p Node
	NodeContainer p2pInternet;
	p2pInternet.Create(1);
	p2pInternet.Add(wifiApNode);

	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	mobility.Install(p2pInternet.Get(0));
	NetDeviceContainer p2pInternetDevices;
	p2pInternetDevices = pointToPoint.Install (p2pInternet);

	//Grid p2p
	NodeContainer p2pGrid;
	p2pGrid.Create(1);
	p2pGrid.Add(espNode);
	mobility.Install(p2pGrid.Get(0));
	NetDeviceContainer p2pGridDevices;
	p2pGridDevices = pointToPoint.Install(p2pGrid);	

	//internet stack
	InternetStackHelper stack;
	stack.Install (wifiApplianceNodes);
	stack.Install (mobileWifiNode);
	stack.Install (p2pInternet);
	stack.Install (p2pGrid);

	//IP addresses
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pInternetInterfaces;
	p2pInternetInterfaces = address.Assign (p2pInternetDevices);

	address.SetBase ("10.0.1.0", "255.255.255.0");
	Ipv4InterfaceContainer p2pGridInterfaces;
	p2pGridInterfaces = address.Assign (p2pGridDevices);

	address.SetBase ("10.0.2.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiInterfaces;
	wifiInterfaces = address.Assign (wifiDevices);
	wifiInterfaces.Add (address.Assign(apDevices));

	Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), 5000)); 	
	//Applications
	//ESP Packet Sink Server
	PacketSinkHelper packetSink("ns3::TcpSocketFactory", sinkLocalAddress);
	ApplicationContainer espServerApp = packetSink.Install(espNode);
	espServerApp.Start(Seconds (2));
	espServerApp.Stop(Seconds (10));

	//Appliance On/Off
	Address remoteAddress (InetSocketAddress (wifiInterfaces.GetAddress (10), 5000)); 
	OnOffHelper onOff("ns3::TcpSocketFactory", remoteAddress);
	ApplicationContainer wifiApplianceApps = onOff.Install(wifiApplianceNodes.Get(0));
	wifiApplianceApps.Start(Seconds (2));
	wifiApplianceApps.Stop(Seconds (10));


	//Animation
	AnimationInterface anim ("animation.xml");
//	anim.SetNodeDescription (wifiApNode, "Access Point");
//	anim.SetNodeDescription (espNode, "ESP");
//	anim.SetNodeDescription (mobileWifiNode, "mobile wifi client");	
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	Simulator::Stop (Seconds (10.0));
	//ASCII Trace
//	AsciiTraceHelper ascii;
//	phy.EnableAsciiAll (ascii.CreateFileStream ("smart-script.tr"));	
	Simulator::Run ();
	Simulator::Destroy ();
	return 0;  
}

