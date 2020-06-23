// serverbrowser.cpp: eihrul's concurrent resolver, and server browser window management
#include <algorithm>
using std::swap;

#include "engine.h"
#include "iengine.h"
#include "SDL_thread.h"
#include "serverinfo.hpp"

struct resolverthread
{
    SDL_Thread *thread;
    const char *query;
    int starttime;
};

struct resolverresult
{
    const char *query;
    ENetAddress address;
};

vector<resolverthread> resolverthreads;
vector<const char *> resolverqueries;
vector<resolverresult> resolverresults;
SDL_mutex *resolvermutex;
SDL_cond *querycond, *resultcond;

#define RESOLVERTHREADS 2
#define RESOLVERLIMIT 3500

int resolverloop(void * data)
{
    resolverthread *rt = (resolverthread *)data;
    SDL_LockMutex(resolvermutex);
    SDL_Thread *thread = rt->thread;
    SDL_UnlockMutex(resolvermutex);
    if(!thread || SDL_GetThreadID(thread) != SDL_ThreadID())
        return 0;
    while(thread == rt->thread)
    {
        SDL_LockMutex(resolvermutex);
        while(resolverqueries.empty()) SDL_CondWait(querycond, resolvermutex);
        rt->query = resolverqueries.pop();
        rt->starttime = totalmillis;
        SDL_UnlockMutex(resolvermutex);

        ENetAddress address = { ENET_HOST_ANY, ENET_PORT_ANY };
        enet_address_set_host(&address, rt->query);

        SDL_LockMutex(resolvermutex);
        if(rt->query && thread == rt->thread)
        {
            resolverresult &rr = resolverresults.add();
            rr.query = rt->query;
            rr.address = address;
            rt->query = NULL;
            rt->starttime = 0;
            SDL_CondSignal(resultcond);
        }
        SDL_UnlockMutex(resolvermutex);
    }
    return 0;
}

void resolverinit()
{
    resolvermutex = SDL_CreateMutex();
    querycond = SDL_CreateCond();
    resultcond = SDL_CreateCond();

    SDL_LockMutex(resolvermutex);
    loopi(RESOLVERTHREADS)
    {
        resolverthread &rt = resolverthreads.add();
        rt.query = NULL;
        rt.starttime = 0;
        rt.thread = SDL_CreateThread(resolverloop, "resolver", &rt);
    }
    SDL_UnlockMutex(resolvermutex);
}

void resolverstop(resolverthread &rt)
{
    SDL_LockMutex(resolvermutex);
    if(rt.query)
    {
#if SDL_VERSION_ATLEAST(2, 0, 2)
        SDL_DetachThread(rt.thread);
#endif
        rt.thread = SDL_CreateThread(resolverloop, "resolver", &rt);
    }
    rt.query = NULL;
    rt.starttime = 0;
    SDL_UnlockMutex(resolvermutex);
}

void resolverclear()
{
    if(resolverthreads.empty()) return;

    SDL_LockMutex(resolvermutex);
    resolverqueries.shrink(0);
    resolverresults.shrink(0);
    loopv(resolverthreads)
    {
        resolverthread &rt = resolverthreads[i];
        resolverstop(rt);
    }
    SDL_UnlockMutex(resolvermutex);
}

void resolverquery(const char *name)
{
    if(resolverthreads.empty()) resolverinit();

    SDL_LockMutex(resolvermutex);
    resolverqueries.add(name);
    SDL_CondSignal(querycond);
    SDL_UnlockMutex(resolvermutex);
}

bool resolvercheck(const char **name, ENetAddress *address)
{
    bool resolved = false;
    SDL_LockMutex(resolvermutex);
    if(!resolverresults.empty())
    {
        resolverresult &rr = resolverresults.pop();
        *name = rr.query;
        address->host = rr.address.host;
        resolved = true;
    }
    else loopv(resolverthreads)
    {
        resolverthread &rt = resolverthreads[i];
        if(rt.query && totalmillis - rt.starttime > RESOLVERLIMIT)
        {
            resolverstop(rt);
            *name = rt.query;
            resolved = true;
        }
    }
    SDL_UnlockMutex(resolvermutex);
    return resolved;
}

bool resolverwait(const char *name, ENetAddress *address)
{
    if(resolverthreads.empty()) resolverinit();

    defformatstring(text, "resolving %s...", name);
    progress(0, text);

    SDL_LockMutex(resolvermutex);
    resolverqueries.add(name);
    SDL_CondSignal(querycond);
    int starttime = SDL_GetTicks(), timeout = 0;
    bool resolved = false;
    for(;;)
    {
        SDL_CondWaitTimeout(resultcond, resolvermutex, 250);
        loopv(resolverresults) if(resolverresults[i].query == name)
        {
            address->host = resolverresults[i].address.host;
            resolverresults.remove(i);
            resolved = true;
            break;
        }
        if(resolved) break;

        timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/RESOLVERLIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RESOLVERLIMIT + 1;
        if(timeout > RESOLVERLIMIT) break;
    }
    if(!resolved && timeout > RESOLVERLIMIT)
    {
        loopv(resolverthreads)
        {
            resolverthread &rt = resolverthreads[i];
            if(rt.query == name) { resolverstop(rt); break; }
        }
    }
    SDL_UnlockMutex(resolvermutex);
    return resolved;
}

#define CONNLIMIT 20000

int connectwithtimeout(ENetSocket sock, const char *hostname, const ENetAddress &address)
{
    defformatstring(text, "connecting to %s:[%d]...", hostname != NULL ? hostname : "local server", address.port);
    progress(0, text);

    ENetSocketSet readset, writeset;
    if(!enet_socket_connect(sock, &address)) for(int starttime = SDL_GetTicks(), timeout = 0; timeout <= CONNLIMIT;)
    {
        ENET_SOCKETSET_EMPTY(readset);
        ENET_SOCKETSET_EMPTY(writeset);
        ENET_SOCKETSET_ADD(readset, sock);
        ENET_SOCKETSET_ADD(writeset, sock);
        int result = enet_socketset_select(sock, &readset, &writeset, 250);
        if(result < 0) break;
        else if(result > 0)
        {
            if(ENET_SOCKETSET_CHECK(readset, sock) || ENET_SOCKETSET_CHECK(writeset, sock))
            {
                int error = 0;
                if(enet_socket_get_option(sock, ENET_SOCKOPT_ERROR, &error) < 0 || error) break;
                return 0;
            }
        }
        timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/CONNLIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) break;
    }

    return -1;
}

vector<serverinfo *> servers;
ENetSocket pingsock = ENET_SOCKET_NULL;
int lastinfo = 0;

void addserver(const char *name, int port, int priority, const char *desc, const char *handle, const char *flags, const char *branch)
{
    loopv(servers) if( servers[i]->is_same( name, port ) ) return;
    if(serverinfo::newserver(name, port, priority, desc, handle, flags, branch) && verbose >= 2)
        conoutf("added server %s (%d) [%s]", name, port, desc);
}
ICOMMAND(0, addserver, "siissss", (char *n, int *p, int *r, char *d, char *h, char *f, char *b), addserver(n, *p > 0 ? *p : SERVER_PORT, *r >= 0 ? *r : 0, d, h, f, b));

VAR(IDF_PERSIST, searchlan, 0, 0, 1);
VAR(IDF_PERSIST, maxservpings, 0, 10, 1000);
VAR(IDF_PERSIST, serverupdateinterval, 0, 15, VAR_MAX);
VAR(IDF_PERSIST, serverbrowser_show_server_platform_and_branch, 0, 0, 1);
VAR(IDF_PERSIST, showserverpriority, 0, 0, 1);
VAR(IDF_PERSIST, showserversortoptions, 0, 0, 1);
VAR(IDF_PERSIST, serverdecay, 0, 20, VAR_MAX);
VAR(0, serverwaiting, 1, serverinfo::WAITING, 0);

void pingservers()
{
    if(pingsock == ENET_SOCKET_NULL)
    {
        pingsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if(pingsock == ENET_SOCKET_NULL)
        {
            lastinfo = totalmillis;
            return;
        }
        enet_socket_set_option(pingsock, ENET_SOCKOPT_NONBLOCK, 1);
        enet_socket_set_option(pingsock, ENET_SOCKOPT_BROADCAST, 1);
    }
    int tmp_millis = totalmillis;
    uchar ping[MAXTRANS];
    ENetBuffer buf = { ping, 0 };
    ucharbuf p(ping, sizeof(ping));
    putint(p, totalmillis ? totalmillis : 1);
    buf.dataLength = p.length();

    static int lastping = 0;
    auto num_servers = servers.size();
    if(lastping >= num_servers) lastping = 0;
    loopi(maxservpings ? min(num_servers, maxservpings) : num_servers)
    {
        if(++lastping >= num_servers) lastping = 0;
        servers[lastping]->ping( pingsock, serverdecay, tmp_millis );
    }

    if(searchlan && serverlanport)
    {
        ENetAddress address;
        address.host = ENET_HOST_BROADCAST;
        address.port = serverlanport;
        enet_socket_send(pingsock, &address, &buf, 1);
    }
    lastinfo = totalmillis;
}

void checkresolver()
{
    int resolving = 0;
    loopv(servers)
    {
        if( servers[i]->need_resolve( resolving ) )
        {
            resolverquery(servers[i]->name());
        }
    }
    if(!resolving) return;

    const char *name = NULL;
    for(;;)
    {
        ENetAddress addr = { ENET_HOST_ANY, ENET_PORT_ANY };
        if(!resolvercheck(&name, &addr)) break;
        loopv(servers)
        {
            if( servers[i]->validate_resolve( name, addr ) )
            {
                break;
            }
        }
    }
}

int lastreset = 0;

void checkpings()
{
    if(pingsock==ENET_SOCKET_NULL) return;
    enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
    ENetBuffer buf;
    ENetAddress addr;
    uchar ping[MAXTRANS];
    buf.data = ping;
    buf.dataLength = sizeof(ping);
    while(enet_socket_wait(pingsock, &events, 0) >= 0 && events)
    {
        int len = enet_socket_receive(pingsock, &addr, &buf, 1);
        if(len <= 0) return;
        serverinfo *si = NULL;
        loopv(servers)
        {
            if( servers[i]->is_same( addr ) )
            {
                si = servers[i];
                break;
            }
        }
        if(!si && searchlan) si = serverinfo::newserver(NULL, addr.port-1, 1, NULL, NULL, NULL, NULL, addr.host);
        if(!si) continue;
        si->update( buf.dataLength, buf.data, serverdecay, totalmillis, lastreset );
    }
}

void refreshservers()
{
    static int lastrefresh = 0;
    if(lastrefresh == totalmillis) return;
    if(totalmillis - lastrefresh > 1000)
    {
        loopv(servers) servers[i]->reset();
        lastreset = totalmillis;
    }
    lastrefresh = totalmillis;

    checkresolver();
    checkpings();
    if(totalmillis - lastinfo >= (serverupdateinterval*1000)/(maxservpings ? max(1, (servers.length() + maxservpings - 1) / maxservpings) : 1)) pingservers();
}

bool reqmaster = false;

void clearservers()
{
    resolverclear();
    servers.deletecontents();
    lastinfo = 0;
}

COMMAND(0, clearservers, "");

#define RETRIEVELIMIT 20000

void retrieveservers(vector<char> &data)
{
    ENetSocket sock = connectmaster(false);
    if(sock == ENET_SOCKET_NULL) return;

    defformatstring(text, "retrieving servers from %s:[%d]", servermaster, servermasterport);
    progress(0, text);

    int starttime = SDL_GetTicks(), timeout = 0;
    const char *req = "update\n";
    int reqlen = strlen(req);
    ENetBuffer buf;
    while(reqlen > 0)
    {
        enet_uint32 events = ENET_SOCKET_WAIT_SEND;
        if(enet_socket_wait(sock, &events, 250) >= 0 && events)
        {
            buf.data = (void *)req;
            buf.dataLength = reqlen;
            int sent = enet_socket_send(sock, NULL, &buf, 1);
            if(sent < 0) break;
            req += sent;
            reqlen -= sent;
            if(reqlen <= 0) break;
        }
        timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/RETRIEVELIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RETRIEVELIMIT + 1;
        if(timeout > RETRIEVELIMIT) break;
    }

    if(reqlen <= 0) for(;;)
    {
        enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
        if(enet_socket_wait(sock, &events, 250) >= 0 && events)
        {
            if(data.length() >= data.capacity()) data.reserve(4096);
            buf.data = data.getbuf() + data.length();
            buf.dataLength = data.capacity() - data.length();
            int recv = enet_socket_receive(sock, NULL, &buf, 1);
            if(recv <= 0) break;
            data.advance(recv);
        }
        timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/RETRIEVELIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RETRIEVELIMIT + 1;
        if(timeout > RETRIEVELIMIT) break;
    }

    if(data.length()) data.add('\0');
    enet_socket_destroy(sock);
}

void sortservers()
{
    serverinfo::sort(servers);
}
COMMAND(0, sortservers, "");

VAR(IDF_PERSIST, autosortservers, 0, 1, 1);
VAR(0, pausesortservers, 0, 0, 1);

void updatefrommaster()
{
    pausesortservers = 0;
    vector<char> data;
    retrieveservers(data);
    if(data.length() && data[0])
    {
        //clearservers();
        execute(data.getbuf());
        if(verbose) conoutf("\faretrieved %d server(s) from master", servers.length());
        else conoutf("\faretrieved list from master successfully");//, servers.length());
    }
    else conoutf("master server not replying");
    refreshservers();
    reqmaster = true;
}
COMMAND(0, updatefrommaster, "");

void updateservers()
{
    if(!reqmaster) updatefrommaster();
    refreshservers();
    if(autosortservers && !pausesortservers) sortservers();
    intret(servers.length());
}
COMMAND(0, updateservers, "");

void writeservercfg()
{
    if(servers.empty()) return;
    stream *f = openutf8file("servers.cfg", "w");
    if(!f) return;
    f->printf("// servers which are connected to or queried get added here automatically\n\n");
    loopv(servers)
    {
        servers[i]->writecfg( *f );
    }
    delete f;
}

VAR(IDF_PERSIST, hideincompatibleservers, 0, 0, 1);
void getservers(int server, int prop, int idx)
{
    if(server < 0)
    {
        // this codes does not really hide stuff, it _removes_ stuff
        if (hideincompatibleservers)
        {
            //servers.erase( std::remove_if(), servers.end()); refuses to work here, dunno why
            //so for now let's keep the waste
            vector<serverinfo *> servers_new;
            std::copy_if( servers.begin(), servers.end(),
                std::back_inserter( servers_new ),serverinfo::server_compatible );
            servers = servers_new;
        }

        intret(servers.length());
    }
    else if(servers.inrange(server))
    {
        servers[server]->cube_get_property( prop, idx );
    }
}
ICOMMAND(0, getserver, "bbb", (int *server, int *prop, int *idx, int *numargs), getservers(*server, *prop, *idx));
