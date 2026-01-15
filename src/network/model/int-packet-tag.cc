#include <stdint.h>
#include "int-packet-tag.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IntPacketTag");

NS_OBJECT_ENSURE_REGISTERED (IntPacketTag);

IntPacketTag::IntPacketTag ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG("IntPacketTag conversion factor " << m_conversionFactor);
}

IntPacketTag::~IntPacketTag ()
{
  NS_LOG_FUNCTION (this);
}

void 
IntPacketTag::SetQueue (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_queue = value;
}

uint32_t
IntPacketTag::GetQueue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void 
IntPacketTag::SetLink (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);
  m_link = value;
}

uint32_t 
IntPacketTag::GetLink (void) const
{
  NS_LOG_FUNCTION (this);
  return m_link;
}

double 
IntPacketTag::GetLinkConverted (void) const
{
  NS_LOG_FUNCTION (this);
  return (double) m_link / m_conversionFactor;
}

uint32_t
IntPacketTag::GetConversionFactor (void) const
{
  NS_LOG_FUNCTION (this);
  return m_conversionFactor;
}

void 
IntPacketTag::SetLock (bool value)
{
  NS_LOG_FUNCTION (this);
  m_lock = value;
}

bool 
IntPacketTag::GetLock (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lock;
}

TypeId
IntPacketTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IntPacketTag")
    .SetParent<Tag> ()
    .SetGroupName ("Network")
    .AddConstructor<IntPacketTag> ()
    .AddAttribute ("ConversionFactor", // WARNING: THIS DOES NOT CURRENTLY WORK
                   "Constant to multiply link utilization by to make it a uint",
                   UintegerValue (100),
                   MakeUintegerAccessor (&IntPacketTag::m_conversionFactor),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

TypeId
IntPacketTag::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint32_t 
IntPacketTag::GetSerializedSize (void) const
{ 
  NS_LOG_FUNCTION (this);
  return sizeof (uint32_t) + sizeof (uint32_t) + sizeof(uint8_t);
}

void 
IntPacketTag::Serialize (TagBuffer i) const
{
  NS_LOG_FUNCTION (this << &i);
  i.WriteU32 (m_queue);
  i.WriteU32 (m_link);
  i.WriteU8 (m_lock);
}
void 
IntPacketTag::Deserialize (TagBuffer i)
{
  NS_LOG_FUNCTION (this<< &i);
  m_queue = i.ReadU32 ();
  m_link = i.ReadU32 ();
  m_lock = i.ReadU8 ();
}
void
IntPacketTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "INT PKTINFO [Queue: " << (uint32_t) m_queue;
  os << ", Link:" << (uint32_t) m_link;
  os << "] ";
}
} // namespace ns3

