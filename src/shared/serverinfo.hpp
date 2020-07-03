#ifndef SERVERINFO_HPP
#define SERVERINFO_HPP

#include <vector>
#include <string>

#include "defs.hpp"

struct serverinfo;

// list of known servers, with or without informations
// TODO:
//   * should be a private static of serverinfo
//   * stop using raw pointers (RAII ftw)
extern vector<serverinfo*> servers;

// code imported, here to reduce build dependencies
extern bool filterstring(char *dst, const char *src, bool newline, bool colour, bool whitespace, bool wsstrip, size_t len);
namespace client
{
    extern bool serverinfocompare(serverinfo const*a, serverinfo const*b);
}

namespace server
{
    extern int getver( int );
}

//this enum lists (some of) the server properties that can be used for sorting.
//The C and C++ specs says and enum starts with the value 0 except if explicitly
//specified.
//It have 2 list of members:
// * row 1: original names, for compat, and maybe to allow some naming support in
//          CS latter
// * row 2: aliases to avoid reapeating 'SINFO::' for nothing.
// TODO: use scoped enums
enum SINFO
{
    SINFO_NONE   ,
    SINFO_STATUS ,
    SINFO_NAME   ,
    SINFO_PORT   ,
    SINFO_QPORT  ,
    SINFO_DESC   ,
    SINFO_MODE   ,
    SINFO_MUTS   ,
    SINFO_MAP    ,
    SINFO_TIME   ,
    SINFO_NUMPLRS,
    SINFO_MAXPLRS,
    SINFO_PING   ,
    SINFO_PRIO   ,
    SINFO_MAX    ,
};

// statuses a server can have, about being joinable
// TODO: use scoped enums
enum SSTAT
{
    SSTAT_OPEN   ,
    SSTAT_LOCKED ,
    SSTAT_PRIVATE,
    SSTAT_FULL   ,
    SSTAT_UNKNOWN,
    SSTAT_MAX    ,
};

// values returned when trying to join a server?
// used in m_attr[5]
enum MM
{
    MM_OPEN    ,
    MM_VETO    ,
    MM_LOCKED  ,
    MM_PRIVATE ,
    MM_PASSWORD,
};

struct serverinfo
{
    // should probably be private, but:
    // WAITING is used in serverbrowser for cubescript exposition
    enum
    {
        MAXPINGS = 3,
        WAITING = INT_MAX
    };

private:
    // list of player/spectators names
    std::vector<std::string> m_players;
    // authentification per player
    std::vector<std::string> m_handles;

    // various server attributes, see cube_get_property() for what I know
    std::vector<int> m_attr;
    // server's UDP/TCP/IP informations
    ENetAddress m_address;
    // description as seen in serverbrowser
    string m_sdesc;

    string m_flags;
    string m_branch;
    string m_authhandle;

    // probably the IP or domain name
    string m_name;
    // name of current map
    string m_map;

    int m_lastping;
    int m_nextping;
    int m_pings[MAXPINGS];
    int m_lastinfo;

    // UDP? port number, probably redundant with m_address
    int m_port;

    int m_priority;
    int m_ping;

    // name resolution state.
    enum
    {
        UNRESOLVED,
        RESOLVING,
        RESOLVED
    } m_resolved;

    // is "servers" sorted?
    static bool s_sortedservers;

    const char* description( void ) const;
    void clearpings();
    void cleanup();
    void addping(int rtt, int millis);
    int protocol_version( void ) const;
    SSTAT server_status( void ) const;
public:
    serverinfo(uint ip, int port, int priority = 0);
    ~serverinfo();

    // marks server as not queried and flags servers(list) as not sorted
    void reset();

    // compare 2 servers according to a sorting "style".
    // returns 0 if style is invalid, abort() in debug mode.
    //
    // Styles NUMPLRS and PRIO are "better if higher".
    // Others are "better if lower".
    // Reverse invert the sorting.
    //
    // returns:
    //   * -1 if *this < other
    //   *  0 if *this = other
    //   *  1 if *this > other
    int compare( serverinfo const& other, int style, bool reverse ) const;
    int compare( serverinfo const& other, SINFO style, bool reverse ) const;

    // return 0 if versions match, -1 if *this > other and 1 otherwise
    int version_compare( serverinfo const& other ) const;

    // returns server name.
    // TODO: remove it. Used by:
    //   _ resolverquery() in engine/serverbrowser.cpp
    //   _ servercompare() in game/client.cpp
    char const* name( void ) const;

    // "return" a property of this server to CS
    // if property < 0: returns the number of properties one can query (4)
    // if property > 3: do nothing.
    // if property > 0:
    //   if index < 0: returns the size of requested array
    //   if index > array.size(): do nothing
    //   if property == 1: returns m_attr[index] (integer)
    //   if property == 2: returns players[index] (result?)
    //   if property == 3: returns handles[index] (result?)
    // if propert == 1:
    //   if index ==  0: returns server_status() (integer)
    //   if index ==  1: returns name (result) (this is probably the IP or domain name)
    //   if index ==  2: returns port (integer)
    //   if index ==  3: returns description or name if no description (result)
    //   if index ==  4: returns map played (result)
    //   if index ==  5: returns number of players (integer)
    //   if index ==  6: returns ping? Guess it's roundtrip time? (integer)
    //   if index ==  7: returns m_lastinfo? guess it's time of last update? (integer)
    //   if index ==  8: returns authhandle?? (result)
    //   if index ==  9: returns flags??  (result)
    //   if index == 10: returns branch?? (resull)
    //   if index == 11: returns priority (integer)
    void cube_get_property( int property, int index );

    // write some current server's attributes into file
    // * m_name
    // * port
    // * priority
    // * description() (escaped)
    // * authhandle (escaped)
    // * flags (escaped)
    // * branch (escaped)
    void writecfg( stream& file ) const;

    // update server's internal informations.
    // According to what exactly is still unknown to me.
    // Candidate for rewrite.
    // TODO: make private?
    //   * only used in serverbrowser.cpp
    void update( size_t len, void const* data, int serverdecay, int totalmillis, int lastreset );

    // methods that allows multi-threaded server name resolution without breaking encapsulation
    bool validate_resolve( char const* name, ENetAddress const& addr );
    bool need_resolve( int& resolving );

    // check if current's server network info (IP/port) match parameter(s)
    bool is_same( ENetAddress const& addr ) const;
    bool is_same( char const* oname, int oport ) const;

    // sends a ping to target
    void ping( ENetSocket& sock, int serverdecay, int millis );

    // register a new server into "servers"
    static serverinfo *newserver(const char *name, int port = SERVER_PORT, int priority = 0, const char *desc = nullptr, const char *handle = nullptr, const char *flags = nullptr, const char *branch = nullptr, uint ip = ENET_HOST_ANY);

    // check if the target is compatible with current client
    static bool server_compatible( serverinfo const* target );

    // sorts "servers"
    static void sort( vector<serverinfo*> &servers );
};

#endif
