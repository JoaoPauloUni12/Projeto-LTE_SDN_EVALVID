/*
 * Copyright (c) 2011-2018 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Authors: Jaume Nin <jaume.nin@cttc.cat>
 *          Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ofswitch13-module.h"
#include "ns3/evalvid-client-server-helper.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

class VideoPriorityController : public OFSwitch13Controller
{
public:
  VideoPriorityController(uint16_t scenario)
    : m_scenario(scenario)
  {
    NS_LOG_UNCOND("VideoPriorityController criado, scenario=" << m_scenario);
  }

  uint16_t GetScenario() const { return m_scenario; }

private:
  uint16_t m_scenario;
};

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeBs,
 * attaches one UE per eNodeB starts a flow for each UE to and from a remote host.
 * It also starts another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE("LenaSimpleEpc");

int
main(int argc, char* argv[])
{
    uint16_t scenario = 0; // 0 = sem SDN, 1 =com SDN
    uint16_t numEnbs = 2;
    uint16_t numUes  = 8;
    Time simTime = Seconds(30);
    double distance = 500.0;
    Time interPacketInterval = MilliSeconds(100);
    
    bool useCa = false;
    bool disableDl = false;
    bool disableUl = false;
    bool disablePl = false;

    std::string videoName = "video1";

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("numEnbs", "Number of eNodeBs", numEnbs);
    cmd.AddValue("numUes", "Number of UEs", numUes);
    cmd.AddValue("simTime", "Total duration of the simulation", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue("interPacketInterval", "Inter packet interval", interPacketInterval);
    cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
    cmd.AddValue("disableDl", "Disable downlink data flows", disableDl);
    cmd.AddValue("disableUl", "Disable uplink data flows", disableUl);
    cmd.AddValue("disablePl", "Disable data flows between peer UEs", disablePl);
    cmd.AddValue("videoName", "Base name of EvalVid video", videoName);
    cmd.AddValue("scenario", "0=sem SDN, 1=com SDN", scenario);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    if (useCa)
    {
        Config::SetDefault("ns3::LteHelper::UseCa", BooleanValue(useCa));
        Config::SetDefault("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue(2));
        Config::SetDefault("ns3::LteHelper::EnbComponentCarrierManager",
                           StringValue("ns3::RrComponentCarrierManager"));
    }

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Política de escalonamento depende do cenário SDN
    if (scenario == 0)
    {
    // Cenário SEM priorização explícita
    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    }
    else
    {
    // Cenário COM SDN: priorização de tráfego (ex.: vídeo) via PF
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    }

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    InternetStackHelper internet;

    if (scenario == 0)
    {
    NS_LOG_UNCOND("Cenario 0: switch sem regras SDN (comportamento padrao)");
    }
    else
    {
    NS_LOG_UNCOND("Cenario 1: aplicar regra SDN para priorizar fluxo EvalVid");
    // TODO: implementar regra OpenFlow no controlador
    }

    LogComponentEnable ("EvalvidClient", LOG_LEVEL_INFO);
    LogComponentEnable ("EvalvidServer", LOG_LEVEL_INFO);

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    internet.Install(remoteHostContainer);

    // Nó do switch SDN (OFSwitch13) e controlador
    NodeContainer ofswitchNodes;
    ofswitchNodes.Create(1);
    Ptr<Node> ofswitch = ofswitchNodes.Get(0);

    NodeContainer controllerNodes;
    controllerNodes.Create(1);
    Ptr<Node> controller = controllerNodes.Get(0);

    // Internet stack no controlador
    internet.Install(controllerNodes);

    // Links P2P: PGW <-> switch <-> remoteHost
    CsmaHelper csmaSw;
    csmaSw.SetChannelAttribute("DataRate", StringValue("1Mbps"));
    csmaSw.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));

    NetDeviceContainer pgwSwDevs = csmaSw.Install(NodeContainer(pgw, ofswitch));
    NetDeviceContainer swRhDevs  = csmaSw.Install(NodeContainer(ofswitch, remoteHost));

    // Configurar OFSwitch13
    //Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper>();
    Ptr<VideoPriorityController> controllerApp = CreateObject<VideoPriorityController>(scenario);
    Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper>();
        
    LogComponentEnable("OFSwitch13Controller", LOG_LEVEL_INFO);
    LogComponentEnable("OFSwitch13InternalHelper", LOG_LEVEL_INFO);
    // portas do switch são as interfaces conectadas ao PGW e ao remoteHost
    NetDeviceContainer switchPorts;
    switchPorts.Add(pgwSwDevs.Get(1));
    switchPorts.Add(swRhDevs.Get(0));

    // mesmo controlador interno nos dois cenários (SDN ativo em ambos)
    of13Helper->InstallController(controller, controllerApp);
    of13Helper->InstallSwitch(ofswitch, switchPorts);
    of13Helper->CreateOpenFlowChannels();

    // Espera a conexão do switch com o controlador antes de instalar a regra
    Simulator::Schedule(Seconds(0.5), [&]() {
    if (scenario == 0)
    {
        NS_LOG_UNCOND("Cenario 0: nao instala regra SDN extra no switch");
        return;
    }

    // dpId do primeiro switch (1) — OFSwitch13InternalHelper usa 1 por padrão
    uint64_t dpId = 1;

    std::string cmd =
        "flow-mod cmd=add,table=0,prio=1000 "
        "eth_type=0x0800,ip_proto=17,udp_dst=8000 "
        "apply:output=all";

    int ret = controllerApp->DpctlExecute(dpId, cmd);
    NS_LOG_UNCOND("Instalando regra SDN de video via dpctl, dpId=" << dpId
                    << " retorno=" << ret);
    });

    // Create the Internet
    PointToPointHelper p2ph;
    if (scenario == 0)
        p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Mb/s")));
    else
        p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("2Mb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numEnbs);
    ueNodes.Create(numUes);

    // eNBs fixos
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    for (uint16_t i = 0; i < numEnbs; ++i)
    {
    enbPositionAlloc->Add(Vector(distance * i, 0.0, 0.0));
    }
    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(enbPositionAlloc);
    mobilityEnb.Install(enbNodes);

    // UEs em movimento entre as células (para gerar handover)
    MobilityHelper mobilityUe;
    mobilityUe.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobilityUe.Install(ueNodes);
    for (uint16_t u = 0; u < numUes; ++u)
    {
    Ptr<ConstantVelocityMobilityModel> mob =
        ueNodes.Get(u)->GetObject<ConstantVelocityMobilityModel>();

    double y = 20.0 * (u % 4);           // 0,20,40,60
    double x0 = (u < 4) ? 0.0 : distance; // 4 UEs perto do eNB0, 4 perto do eNB1
    mob->SetPosition(Vector(x0, y, 0.0));

    // metade vai para a outra célula (handover), metade anda “contra”
    double vx = (u % 2 == 0) ? 5.0 : -5.0;
    mob->SetVelocity(Vector(vx, 0.0, 0.0));
    }



    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);


    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }
    uint16_t bgPort = 9000;

    // Gerador UDP no remoteHost enviando para todos os UEs
    OnOffHelper onoff("ns3::UdpSocketFactory",
                    Address(InetSocketAddress(Ipv4Address("7.0.0.0"), bgPort)));
    if (scenario == 0)
        onoff.SetAttribute("DataRate", StringValue("6Mb/s"));
    else
        onoff.SetAttribute("DataRate", StringValue("4Mb/s"));
    // tráfego pesado
    onoff.SetAttribute("PacketSize", UintegerValue(1200));

    ApplicationContainer bgApps;
    for (uint32_t u = 0; u < ueIpIface.GetN(); ++u)
    {
    AddressValue remoteAddr(
        InetSocketAddress(ueIpIface.GetAddress(u), bgPort));
    onoff.SetAttribute("Remote", remoteAddr);
    bgApps.Add(onoff.Install(remoteHost));
    }

    // Sink UDP em cada UE
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                        Address(InetSocketAddress(Ipv4Address::GetAny(), bgPort)));
    ApplicationContainer sinkApps = sink.Install(ueNodes);

    // Ativa o tráfego de fundo durante quase toda a simulação
    bgApps.Start(Seconds(2.0));
    bgApps.Stop(simTime - Seconds(1.0));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(simTime);

    // Interfaces X2 entre eNBs para permitir handover
    lteHelper->AddX2Interface(enbNodes);

    // Attach automático: o helper escolhe o melhor eNB e faz handover quando a SINR mudar
    lteHelper->Attach(ueLteDevs);

    // === Bearers dedicados para vídeo VIP (apenas cenário 1) ===
    if (scenario == 1)
    {
    EpsBearer videoBearer(EpsBearer::GBR_CONV_VIDEO); // QCI típico de vídeo em tempo real

    for (uint16_t u = 0; u < 2 && u < ueLteDevs.GetN(); ++u)  // UEs 0 e 1 são VIP
    {
        Ptr<NetDevice> dev = ueLteDevs.Get(u);
        lteHelper->ActivateDedicatedEpsBearer(dev, videoBearer, EpcTft::Default());
    }
    }


    uint16_t basePort = 8000;
    ApplicationContainer serverApps, clientApps;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
    uint16_t port = basePort + u;

    bool isVip = (u < 2);
    bool useHarbour = isVip; // UE par: harbour, ímpar: crew
    std::string baseName = useHarbour ? "harbour" : "crew";

    EvalvidServerHelper server(port);
    server.SetAttribute("SenderTraceFilename", StringValue(useHarbour ? "st_harbour.st" : "st_crew.st"));

    std::ostringstream sdname;
    sdname << baseName << "_sd_ue" << u << "_s" << scenario;
    server.SetAttribute("SenderDumpFilename", StringValue(sdname.str()));

    ApplicationContainer sa = server.Install(remoteHost);
    sa.Start(Seconds(5.0));
    sa.Stop (simTime - Seconds(5.0));
    serverApps.Add(sa);

    EvalvidClientHelper client(remoteHostAddr, port);
    std::ostringstream rdname;
    rdname << baseName << "_rd_ue" << u << "_s" << scenario;
    client.SetAttribute("ReceiverDumpFilename", StringValue(rdname.str()));

    ApplicationContainer ca = client.Install(ueNodes.Get(u));
    ca.Start(Seconds(5.0));
    ca.Stop(simTime - Seconds(5.0));
    clientApps.Add(ca);
    }

    lteHelper->EnableTraces();

    // ===== FlowMonitor =====
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(simTime);
    Simulator::Run();

    // ===== Processa métricas QoS =====
    Ptr<Ipv4FlowClassifier> classifier =
    DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    // nomes de arquivos dependem do cenário
    std::ostringstream prefix;
    prefix << "s" << scenario << "_";

    std::ofstream fVazao((prefix.str() + "QoS_vazao.txt").c_str());
    std::ofstream fPerda((prefix.str() + "QoS_perda.txt").c_str());
    std::ofstream fJitter((prefix.str() + "QoS_jitter.txt").c_str());
    std::ofstream fDelay((prefix.str() + "QoS_delay.txt").c_str());
    std::vector<double> ueThroughput(numUes, 0.0);

    for (auto &it : stats)
    {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it.first);

    double timeFirst = it.second.timeFirstTxPacket.GetSeconds();
    double timeLast  = it.second.timeLastRxPacket.GetSeconds();
    double duration  = (timeLast - timeFirst > 0.0) ? (timeLast - timeFirst) : 1e-9;
    double rxBytes   = (double) it.second.rxBytes;
    double throughput = (rxBytes * 8.0) / (duration * 1e6); // Mbps
    
     NS_LOG_UNCOND("Flow " << it.first
      << " src=" << t.sourceAddress << ":" << t.sourcePort
      << " dst=" << t.destinationAddress << ":" << t.destinationPort
      << " thr=" << throughput << " Mbps");
    
    // só considerar fluxos de vídeo (destino porta 8000–8007)
    // só considerar fluxos de vídeo downlink (source port 8000–8000+numUes-1)
    if (t.sourcePort >= 8000 && t.sourcePort < 8000 + numUes)
    {
    for (uint32_t u = 0; u < ueIpIface.GetN(); ++u)
    {
        if (ueIpIface.GetAddress(u) == t.destinationAddress)
        {
        ueThroughput[u] += throughput;
        }
    }
    }

    double lossRatio = (double) it.second.lostPackets /
                        std::max(1u, it.second.txPackets) * 100.0;
    double jitterMean = (it.second.rxPackets > 0)
                            ? (it.second.jitterSum.GetSeconds() / it.second.rxPackets)
                            : 0.0;
    double delayMean = (it.second.rxPackets > 0)
                        ? (it.second.delaySum.GetSeconds() / it.second.rxPackets)
                        : 0.0;
    fVazao << "Flow " << it.first << " (" << t.sourceAddress << " -> "
            << t.destinationAddress << "): " << throughput << " Mbps\n";
    fPerda << "Flow " << it.first << ": " << lossRatio << " % ("
            << it.second.lostPackets << "/" << it.second.txPackets << ")\n";
    fJitter << "Flow " << it.first << ": " << jitterMean << " s\n";
    fDelay << "Flow " << it.first << " ("
       << t.sourceAddress << ":" << t.sourcePort << " -> "
       << t.destinationAddress << ":" << t.destinationPort << "): "
       << delayMean << " s\n";
    }
    // grava largura de banda por UE como no modelo do professor
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        std::ostringstream fname;
        fname << "s" << scenario << "_QoS_drops_largura_ue" << (u + 1) << ".txt";
        std::ofstream fLB(fname.str().c_str());
        fLB << "UE " << (u + 1) << " throughput total: "
            << ueThroughput[u] << " Mbps\n";
        fLB.close();
    }

    fVazao.close();
    fPerda.close();
    fJitter.close();
    fDelay.close();

    // opcional: XML geral
    std::string xmlName = prefix.str() + "QoS_flowmonitor.xml";
    monitor->SerializeToXmlFile(xmlName, true, true);

    Simulator::Destroy();
    return 0;
}
