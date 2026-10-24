/* Offline platform transport. ioquake3's actual loopback queues remain in
 * net_chan.c; no external address is resolved or packet sent here.
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

qboolean NET_CompareBaseAdrMask(netadr_t a, netadr_t b, int mask)
{
  (void)mask;
  return a.type == NA_LOOPBACK && b.type == NA_LOOPBACK;
}

qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b)
{
  return NET_CompareBaseAdrMask(a, b, -1);
}

qboolean NET_CompareAdr(netadr_t a, netadr_t b)
{
  return NET_CompareBaseAdr(a, b);
}

qboolean NET_IsLocalAddress(netadr_t address)
{
  return address.type == NA_LOOPBACK;
}

const char *NET_AdrToString(netadr_t address)
{
  if (address.type == NA_LOOPBACK)
    return "loopback";
  if (address.type == NA_BOT)
    return "bot";
  return "unavailable";
}

const char *NET_AdrToStringwPort(netadr_t address)
{
  return NET_AdrToString(address);
}

qboolean Sys_StringToAdr(const char *name, netadr_t *address, netadrtype_t family)
{
  (void)name;
  (void)family;
  memset(address, 0, sizeof(*address));
  address->type = NA_BAD;
  return qfalse;
}

void Sys_SendPacket(int length, const void *data, netadr_t to)
{
  (void)length;
  (void)data;
  (void)to;
  Com_Printf("External networking is unavailable in this Pyxis build\n");
}

qboolean Sys_IsLANAddress(netadr_t address)
{
  return NET_IsLocalAddress(address);
}

void Sys_ShowIP(void)
{
  Com_Printf("Offline build: only the in-process loopback transport is available\n");
}

void NET_Config(qboolean enable)
{
  (void)enable;
}

void NET_Restart_f(void)
{
  Sys_ShowIP();
}

void NET_Init(void)
{
  Cvar_Get("net_enabled", "0", CVAR_ROM);
  Cvar_ForceReset("net_enabled");
  Cmd_AddCommand("net_restart", NET_Restart_f);
  Sys_ShowIP();
}

void NET_Shutdown(void)
{
  Cmd_RemoveCommand("net_restart");
}

void NET_JoinMulticast6(void)
{
  Com_Printf("Multicast is unavailable in this Pyxis build\n");
}

void NET_LeaveMulticast6(void)
{
}

void NET_Sleep(int milliseconds)
{
  Sys_Sleep(milliseconds);
}
