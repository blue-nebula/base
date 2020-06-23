#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <zlib.h>
#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <GL/gl.h>

using std::swap;
#include "tools.h"
#include "command.h"
#include "serverinfo.hpp"

void serverinfo::clearpings()
{
    m_ping = WAITING;
    loopk(MAXPINGS)
    {
        pings[k] = WAITING;
    }
    nextping = 0;
    lastping = lastinfo = -1;
}

void serverinfo::cleanup()
{
    clearpings();
    attr.clear();
    players.clear();
    handles.clear();
    numplayers = 0;
}

void serverinfo::addping(int rtt, int millis)
{
    if(millis >= lastping)
    {
        lastping = -1;
    }
    pings[nextping] = rtt;
    nextping = (nextping+1)%MAXPINGS;
    int numpings = 0;
    int totalpings = 0;
    loopk(MAXPINGS)
    {
        if(pings[k] != WAITING)
        {
            totalpings += pings[k];
            numpings++;
        }
    }
    m_ping = numpings ? totalpings/numpings : WAITING;
}

// return description if not empty, name otherwise.
const char* serverinfo::description( void ) const
{
    return sdesc[0] ? sdesc : m_name;
}

// returns protocol version
//
// -1 if server still have no informations
// 0x7FFF if server uses VERSION_GAME on attr[0] (for older protocol compat?)
// attr[0] clamped in range [0,0x7FFE]
int serverinfo::protocol_version( void ) const
{
    if( false
        || m_address.host == ENET_HOST_ANY
        || m_ping >= serverinfo::WAITING
        || attr.empty()
      )
    {
        return -1;
    }
    return attr[0] == VERSION_GAME ? 0x7fff : clamp( attr[0], 0, 0x7FFF - 1 );
}

// return "status" of server, aka: can it be joined
SSTAT serverinfo::server_status( void ) const
{
    if( attr.size() > 4 && numplayers >= attr[4] )
    {
        return SSTAT_FULL;
    }
    if(attr.size() <= 5)
    {
        return SSTAT_UNKNOWN;
    }
    switch(attr[5])
    {
        case MM_LOCKED:
            return SSTAT_LOCKED;
        case MM_PRIVATE:
        case MM_PASSWORD:
            return SSTAT_PRIVATE;
        default:
            return SSTAT_OPEN;
    }
}

// start of public methods definitions

serverinfo::serverinfo(uint ip, int port, int priority)
 : port(port), priority(priority), numplayers(0), resolved(ip==ENET_HOST_ANY ? UNRESOLVED : RESOLVED)
{
    m_name[0] = map[0] = sdesc[0] = authhandle[0] = flags[0] = branch[0] = '\0';
    m_address.host = ip;
    m_address.port = port+1;
    clearpings();
}

serverinfo::~serverinfo()
{
    cleanup();
}

void serverinfo::reset()
{
    lastping = lastinfo = -1;
    sortedservers = false;
}

int serverinfo::compare( serverinfo const& other, int style, bool reverse ) const
{
    assert( SINFO_NONE < style && style < SINFO_MAX );
    if( style < 0 || style > SINFO_MAX )
    {
        return 0;
    }
    return compare( other, static_cast<SINFO>( style ), reverse );
}

int serverinfo::compare( serverinfo const& other, SINFO style, bool reverse ) const
{
    int ac = 0;
    int bc = 0;
    int index = 0;
    int comp  = 0;
    switch(style)
    {
        case SINFO_DESC:
            if( ( comp = strcmp(sdesc, other.sdesc) ) )
            {
                return reverse ? 0 - comp: comp ;
            }
            return 0;
        case SINFO_MAP:
            if( ( comp = strcmp(map, other.map) ) )
            {
                return reverse ? 0 - comp : comp;
            }
            return 0;
        case SINFO_MODE:
            index = 1;
        case SINFO_MUTS:
            index = 2;
            break;
        case SINFO_TIME:
            index = 3;
            break;
        case SINFO_MAXPLRS:
            index = 4;
            break;
        case SINFO_STATUS:
            ac = server_status();
            bc = other.server_status();
            break;
        case SINFO_NUMPLRS:
            ac = numplayers;
            bc = other.numplayers;
            break;
        case SINFO_PING:
            ac = m_ping;
            bc = other.m_ping;
            break;
        case SINFO_PRIO:
            ac = priority;
            bc = other.priority;
            break;
        default:
            assert( false && "unknown style" );
            return 0;
    }
    if( index != 0 )
    {
        ac =       attr.size() > index ?       attr[index] : 0;
        bc = other.attr.size() > index ? other.attr[index] : 0;
    }

    if( ac == bc )
    {
        return 0;
        bool higher_is_better = style != SINFO_NUMPLRS && style != SINFO_PRIO;
        if( higher_is_better != reverse ? ac < bc : ac > bc )
        {
            return -1;
        }
        if( higher_is_better != reverse ? ac > bc : ac < bc )
        {
            return  1;
        }
    }
    return 0;
}

int serverinfo::version_compare( serverinfo const& other ) const
{
    int ac = protocol_version();
    int bc = other.protocol_version();

    if(ac > bc)
    {
        return -1;
    }
    if(ac < bc)
    {
        return 1;
    }
    return 0;
}

char const* serverinfo::name( void ) const
{
    return m_name;
}

void serverinfo::cube_get_property( int prop, int idx )
{
    if(prop < 0)
    {
        intret(4);
        return;
    }
    switch(prop)
    {
        case 0:
            if(idx < 0)
            {
                intret(11);
                return;
            }
            switch(idx)
            {
                case 0:
                    intret(server_status());
                    break;
                case 1:
                    result(m_name);
                    break;
                case 2:
                    intret(port);
                    break;
                case 3:
                    result(description());
                    break;
                case 4:
                    result(map);
                    break;
                case 5:
                    intret(numplayers);
                    break;
                case 6:
                    intret(m_ping);
                    break;
                case 7:
                    intret(lastinfo);
                    break;
                case 8:
                    result(authhandle);
                    break;
                case 9:
                    result(flags);
                    break;
                case 10:
                    result(branch);
                    break;
                case 11:
                    intret(priority);
                    break;
            }
            return;
        case 1:
            if(idx < 0)
            {
                intret(attr.size());
            }
            else if( idx < attr.size())
            {
                intret(attr[idx]);
            }
            break;
        case 2:
            if(idx < 0)
            {
                intret(players.size());
                return;
            }
            if( idx < players.size())
            {
                result(players[idx].data());
            }
            return;
        case 3:
            if(idx < 0)
            {
                intret(handles.size());
                return;
            }
            if( idx < handles.size())
            {
                result(handles[idx].data());
            }
            return;
    }
}

void serverinfo::writecfg( stream& file ) const
{
    file.printf("addserver %s %d %d %s %s %s %s\n",
        m_name, port, priority,
        escapestring(description()),
        escapestring(authhandle),
        escapestring(flags),
        escapestring(branch)
    );
}

void serverinfo::update( size_t len, void const* data, int serverdecay, int totalmillis, int lastreset )
{
    char text[MAXTRANS];
    // const_cast: ucharbuf p is only read, but have no constness info
    ucharbuf p( static_cast<uint8_t*>( const_cast<void*>( data ) ), len);
    int millis = getint(p);
    int rtt = clamp(totalmillis - millis, 0, min(serverdecay*1000, totalmillis));
    if(millis >= lastreset && rtt < serverdecay*1000)
    {
        addping(rtt, millis);
    }
    lastinfo = totalmillis;
    numplayers = getint(p);
    int numattr = getint(p);
    attr.clear();
    loopj(numattr)
    {
        attr.emplace_back(getint(p));
    }
    int gver = attr.empty() ? 0 : attr[0];
    getstring(text, p);
    filterstring(map, text, false, true, true, false, sizeof( map ));
    getstring(text, p);
    filterstring(sdesc, text, true, true, true, false, MAXSDESCLEN+1);
    players.clear();
    handles.clear();
    if(gver >= 227)
    {
        getstring(text, p);
        filterstring(branch, text, true, true, true, false, MAXBRANCHLEN+1);
    }
    loopi(numplayers)
    {
        if(p.overread())
        {
            break;
        }
        getstring(text, p);
        players.emplace_back(newstring(text));
    }
    if(gver >= 225)
    {
        loopi(numplayers)
        {
            if(p.overread())
            {
                break;
            }
            getstring(text, p);
            handles.emplace_back(newstring(text));
        }
    }
    sortedservers = false;
}

bool serverinfo::validate_resolve( char const* name, ENetAddress const& addr )
{
    if( m_name != name )
    {
        return false;
    }
    resolved = serverinfo::RESOLVED;
    m_address.host = addr.host;
    return true;
}

bool serverinfo::need_resolve( int& resolving )
{
    if( resolved == RESOLVED || m_address.host != ENET_HOST_ANY )
    {
        return false;
    }
    int ret = resolved == UNRESOLVED;
    ++resolving;
    resolved = RESOLVING;
    return ret;
}

bool serverinfo::is_same( ENetAddress const& addr ) const
{
    return m_address.host == addr.host && m_address.port == addr.port;
}

bool serverinfo::is_same( char const* oname, int oport ) const
{
    return 0 == strcmp( m_name, oname ) && port == oport;
}

void serverinfo::ping( ENetSocket& sock, int serverdecay, int millis )
{
    if( m_address.host == ENET_HOST_ANY )
    {
      return;
    }

    //should be more than enouhg to store a "compressed" int
    uchar ping[12];
    ucharbuf ubuf( ping, sizeof( ping ) );
    putint( ubuf, millis ? millis : 1 );
    ENetBuffer buf = { ping, static_cast<size_t>( ubuf.length() ) };
    enet_socket_send( sock, &m_address, &buf, 1 );
    if(lastping >= 0 && millis - lastping >= serverdecay * 1000)
    {
        cleanup();
    }
    if(lastping < 0)
    {
        lastping = millis ? millis : 1;
    }
}

// should probably call a private constructor
serverinfo *serverinfo::newserver(const char *name, int port, int priority, const char *desc, const char *handle, const char *flags, const char *branch, uint ip)
{
    serverinfo *si = new serverinfo(ip, port, priority);
    assert( si );

    if(name)
    {
        copystring(si->m_name, name);
    }
    else if(ip == ENET_HOST_ANY || enet_address_get_host_ip(&si->m_address, si->m_name, sizeof(si->m_name)) < 0)
    {
        delete si;
        return nullptr;
    }
    if(desc && *desc)
    {
        copystring(si->sdesc, desc, MAXSDESCLEN+1);
    }
    if(handle && *handle)
    {
        copystring(si->authhandle, handle);
    }
    if(flags && *flags)
    {
        copystring(si->flags, flags);
    }
    if(branch && *branch)
    {
        copystring(si->branch, branch, MAXBRANCHLEN+1);
    }

    servers.emplace_back( si );
    sortedservers = false;

    return si;
}

bool serverinfo::server_compatible( serverinfo const* si )
{
    return si->attr.empty() || si->attr[0] == server::getver(1);
}

void serverinfo::sort( vector<serverinfo*> &servers )
{
    if( !sortedservers )
    {
        std::sort( servers.begin(), servers.end(), client::serverinfocompare );
        sortedservers = true;
    }
}

bool serverinfo::sortedservers = false;
