#pragma once

#include "network.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "rooms-plugin\RoomsPlugin.h"

#define RAKNET_TYPE_DIRECT_JOIN "Direct-IP  Join"
#define RAKNET_TYPE_DIRECT_HOST "Direct-IP  Host"
#define RAKNET_TYPE_NAT_HOST "RAKNet NAT Host"
#define RAKNET_TYPE_NAT_JOIN "RAKNet NAT Join"

constexpr auto MASTER_SERVER_GAME_ID = "ATD";
constexpr auto MASTER_SERVER_PORT = 60013;
constexpr auto MASTER_NAT_SERVER_PORT = 60014;
constexpr auto MASTER_SERVER_ADDRESS = "127.0.0.1";

#pragma pack(push, 1)
namespace RakNet {
class NatPunchthroughClient;
}

class RAKNetRoomCallbacks;

struct RAKNetworkPeer {
    BYTE netID;
    ULONG ID;
    RakNet::RakNetGUID guid;
    RakNet::SystemAddress address;
};
#pragma pack(pop)
struct RAKNetworkPlayer : public SBNetworkPlayer {
    // ULONG ID;
    RakNet::RakNetGUID peer;
};
struct RAKSessionInfo : public SBSessionInfo {
    // ULONG hostID;
    // char sessionName[26];
    RakNet::RakNetGUID address;
};

class RAKNetNetwork : public BaseNetworkType, public IServerSearchable {
  public:
    RAKNetNetwork();

    void Initialize() override;
    void Disconnect() override;
    bool IsSessionFinished() override;
    bool IsInSession() override;
    SLONG GetMessageCount() override;
    bool Connect(const char *) override;
    bool CreateSession(SBNetworkCreation *) override;
    void CloseSession() override;
    ULONG GetLocalPlayerID() override;
    bool Send(BUFFER<UBYTE> &, ULONG, ULONG, bool) override;
    bool Receive(UBYTE **, ULONG &) override;
    SBList<SBNetworkPlayer *> *GetAllPlayers() override;
    bool IsServerSearchable() override;
    IServerSearchable *GetServerSearcher() override;

    // Server Searchable:

    void LoginMasterServer();
    void RetrieveRoomList();
    SBList<SBStr> *GetSessionListAsync() override;
    bool StartGetSessionListAsync() override;
    bool JoinSession(const SBStr &sessionName, SBStr nickname) override;

  private:
    /// <summary>
    /// Polls the interface until a connection was established
    /// </summary>
    /// <param name="peerInterface">The interface to check fo a successful connection</param>
    /// <returns>true - when a connection was established</returns>
    bool AwaitConnection(RakNet::RakPeerInterface *peerInterface, bool isAnotherPeer);

    RakNet::SystemAddress mServer;
    RakNet::RakNetGUID mHost;
    RakNet::RakPeerInterface *mMaster = nullptr;
    RakNet::NatPunchthroughClient *mNATPlugin = nullptr;
    bool isNATMode = false;

    SBStr *test = nullptr;

    SBList<RakNet::Packet *> mPackets;

    RakNet::RoomsPlugin *mRoomsPluginClient = nullptr;
    RakNet::RakPeerInterface *mServerBrowserPeer = nullptr;
    RAKNetRoomCallbacks *mRoomCallbacks;
    bool mIsConnectingToMaster = false;

    /// <summary>
    /// Starts to retrieve a list of clients that are connected to the specified master server
    /// Response will be send to the mServerSearch peer
    /// </summary>
    /// <param name="serverGuid">The GUID of the master server</param>
    void RequestHostedClients(RakNet::RakNetGUID serverGuid);

    /// <summary>
    /// Connects blocking to the master server and on successful connection will attempt a login with the username
    /// </summary>
    /// <returns>true - on success, otherwise false</returns>
    bool ConnectToMasterServer();

    // Server Search network elements:
    RakNet::RakPeerInterface *mRoomClient = nullptr;
};
