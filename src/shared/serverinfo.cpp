#include <algorithm>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <zlib.h>
#include <SDL.h>
#include <enet/enet.h>

#ifndef STANDALONE
  #ifdef __APPLE__
    #define GL_GLEXT_LEGACY
    #define __glext_h_
    #include <OpenGL/gl.h>
    #define main SDL_main
  #else
    #include <SDL_opengl.h>
  #endif
#endif

using std::swap;
#include "tools.h"
#include "command.h"
#include "serverinfo.hpp"

void serverinfo::clearpings()
{
    m_ping = WAITING;
    loopk(MAXPINGS)
    {
        m_pings[k] = WAITING;
    }
    m_nextping = 0;
    m_lastping = m_lastinfo = -1;
}

void serverinfo::cleanup()
{
    clearpings();
    m_attr.clear();
    m_players.clear();
    m_handles.clear();
}

void serverinfo::addping(int rtt, int millis)
{
    if(millis >= m_lastping)
    {
        m_lastping = -1;
    }
    m_pings[m_nextping] = rtt;
    m_nextping = (m_nextping+1)%MAXPINGS;
    int numpings = 0;
    int totalpings = 0;
    loopk(MAXPINGS)
    {
        if(m_pings[k] != WAITING)
        {
            totalpings += m_pings[k];
            numpings++;
        }
    }
    m_ping = numpings ? totalpings/numpings : WAITING;
}

// return description if not empty, name otherwise.
const char* serverinfo::description( void ) const
{
    return m_sdesc[0] ? m_sdesc : m_name;
}

// returns protocol version
//
// -1 if server still have no informations
// 0x7FFF if server uses VERSION_GAME on m_attr[0] (for older protocol compat?)
// m_attr[0] clamped in range [0,0x7FFE]
int serverinfo::protocol_version( void ) const
{
    if( false
        || m_address.host == ENET_HOST_ANY
        || m_ping >= serverinfo::WAITING
        || m_attr.empty()
      )
    {
        return -1;
    }
    return m_attr[0] == VERSION_GAME ? 0x7fff : clamp( m_attr[0], 0, 0x7FFF - 1 );
}

// return "status" of server, aka: can it be joined
SSTAT serverinfo::server_status( void ) const
{
    if( m_attr.size() > 4 && m_players.size() >= m_attr[4] )
    {
        return SSTAT_FULL;
    }
    if(m_attr.size() <= 5)
    {
        return SSTAT_UNKNOWN;
    }
    switch(m_attr[5])
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
 : m_port(port), m_priority(priority), m_resolved(ip==ENET_HOST_ANY ? UNRESOLVED : RESOLVED)
{
    m_name[0] = m_map[0] = m_sdesc[0] = m_authhandle[0] = m_flags[0] = m_branch[0] = '\0';
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
    m_lastping = m_lastinfo = -1;
    s_sortedservers = false;
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
    size_t index = 0;
    int comp  = 0;
    switch(style)
    {
        case SINFO_DESC:
            if( ( comp = strcmp(m_sdesc, other.m_sdesc) ) )
            {
                return reverse ? 0 - comp: comp ;
            }
            return 0;
        case SINFO_MAP:
            if( ( comp = strcmp(m_map, other.m_map) ) )
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
            ac = m_players.size();
            bc = other.m_players.size();
            break;
        case SINFO_PING:
            ac = m_ping;
            bc = other.m_ping;
            break;
        case SINFO_PRIO:
            ac = m_priority;
            bc = other.m_priority;
            break;
        default:
            assert( false && "unknown style" );
            return 0;
    }
    if( index != 0 )
    {
        ac =       m_attr.size() > index ?       m_attr[index] : 0;
        bc = other.m_attr.size() > index ? other.m_attr[index] : 0;
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
                    intret(m_port);
                    break;
                case 3:
                    result(description());
                    break;
                case 4:
                    result(m_map);
                    break;
                case 5:
                    intret(m_players.size());
                    break;
                case 6:
                    intret(m_ping);
                    break;
                case 7:
                    intret(m_lastinfo);
                    break;
                case 8:
                    result(m_authhandle);
                    break;
                case 9:
                    result(m_flags);
                    break;
                case 10:
                    result(m_branch);
                    break;
                case 11:
                    intret(m_priority);
                    break;
            }
            return;
        case 1:
            if(idx < 0)
            {
                intret(m_attr.size());
            }
            else if( static_cast<size_t>( idx ) < m_attr.size())
            {
                intret(m_attr[idx]);
            }
            break;
        case 2:
            if(idx < 0)
            {
                intret(m_players.size());
                return;
            }
            if( static_cast<size_t>( idx ) < m_players.size())
            {
                result(m_players[idx].data());
            }
            return;
        case 3:
            if(idx < 0)
            {
                intret(m_handles.size());
                return;
            }
            if( static_cast<size_t>( idx ) < m_handles.size())
            {
                result(m_handles[idx].data());
            }
            return;
    }
}

void serverinfo::writecfg( stream& file ) const
{
    file.printf("addserver %s %d %d %s %s %s %s\n",
        m_name, m_port, m_priority,
        escapestring(description()),
        escapestring(m_authhandle),
        escapestring(m_flags),
        escapestring(m_branch)
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
    m_lastinfo = totalmillis;
    auto numplayers = getint(p);
    int numattr = getint(p);
    m_attr.clear();
    loopj(numattr)
    {
        m_attr.emplace_back(getint(p));
    }
    int gver = m_attr.empty() ? 0 : m_attr[0];
    getstring(text, p);
    filterstring(m_map, text, false, true, true, false, sizeof( m_map ));
    getstring(text, p);
    filterstring(m_sdesc, text, true, true, true, false, MAXSDESCLEN+1);
    m_players.clear();
    m_handles.clear();
    if(gver >= 227)
    {
        getstring(text, p);
        filterstring(m_branch, text, true, true, true, false, MAXBRANCHLEN+1);
    }
    loopi(numplayers)
    {
        if(p.overread())
        {
            break;
        }
        getstring(text, p);
        m_players.emplace_back(newstring(text));
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
            m_handles.emplace_back(newstring(text));
        }
    }
    s_sortedservers = false;
}

bool serverinfo::validate_resolve( char const* name, ENetAddress const& addr )
{
    if( m_name != name )
    {
        return false;
    }
    m_resolved = serverinfo::RESOLVED;
    m_address.host = addr.host;
    return true;
}

bool serverinfo::need_resolve( int& resolving )
{
    if( m_resolved == RESOLVED || m_address.host != ENET_HOST_ANY )
    {
        return false;
    }
    int ret = m_resolved == UNRESOLVED;
    ++resolving;
    m_resolved = RESOLVING;
    return ret;
}

bool serverinfo::is_same( ENetAddress const& addr ) const
{
    return m_address.host == addr.host && m_address.port == addr.port;
}

bool serverinfo::is_same( char const* oname, int oport ) const
{
    return 0 == strcmp( m_name, oname ) && m_port == oport;
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
#ifdef WIN32
    ENetBuffer buf = { static_cast<size_t>( ubuf.length() ), ping };
#else
    ENetBuffer buf = { ping, static_cast<size_t>( ubuf.length() ) };
#endif
    enet_socket_send( sock, &m_address, &buf, 1 );
    if(m_lastping >= 0 && millis - m_lastping >= serverdecay * 1000)
    {
        cleanup();
    }
    if(m_lastping < 0)
    {
        m_lastping = millis ? millis : 1;
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
        copystring(si->m_sdesc, desc, MAXSDESCLEN+1);
    }
    if(handle && *handle)
    {
        copystring(si->m_authhandle, handle);
    }
    if(flags && *flags)
    {
        copystring(si->m_flags, flags);
    }
    if(branch && *branch)
    {
        copystring(si->m_branch, branch, MAXBRANCHLEN+1);
    }

    servers.emplace_back( si );
    s_sortedservers = false;

    return si;
}

bool serverinfo::server_compatible( serverinfo const* si )
{
    return si->m_attr.empty() || si->m_attr[0] == server::getver(1);
}

void serverinfo::sort( vector<serverinfo*> &servers )
{
    if( !s_sortedservers )
    {
        std::sort( servers.begin(), servers.end(), client::serverinfocompare );
        s_sortedservers = true;
    }
}

bool serverinfo::s_sortedservers = false;
