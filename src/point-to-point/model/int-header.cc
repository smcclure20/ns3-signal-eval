#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "int-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IntHeader");

NS_OBJECT_ENSURE_REGISTERED (IntHeader);

IntHeader::IntHeader ()
{
}

IntHeader::~IntHeader ()
{
}

TypeId
IntHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IntHeader")
    .SetParent<Header> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<IntHeader> ()
  ;
  return tid;
}

TypeId
IntHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
IntHeader::Print (std::ostream &os) const
{
  os << "INT Header: queue=" << m_queue << ";link=" << m_link; 
}

uint32_t
IntHeader::GetSerializedSize (void) const
{
  return 2 * sizeof(uint32_t) + sizeof(uint8_t);
}

void
IntHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_queue);
  start.WriteHtonU32 (m_link);
  start.WriteU8 (m_lock);
}

uint32_t
IntHeader::Deserialize (Buffer::Iterator start)
{
  m_queue = start.ReadNtohU32 ();
  m_link = start.ReadNtohU32 ();
  m_lock = start.ReadU8 ();
  return GetSerializedSize ();
}

void
IntHeader::SetQueue (uint32_t value)
{
  m_queue = value;
}

uint32_t
IntHeader::GetQueue (void)
{
  return m_queue;
}

void
IntHeader::SetLink (uint32_t value)
{
  m_link = value;
}

uint32_t
IntHeader::GetLink (void)
{
  return m_link;
}

void
IntHeader::SetLock (bool value)
{
  m_lock = value;
}

bool
IntHeader::GetLock (void)
{
  return m_lock;
}


} // namespace ns3
