#ifndef INT_PACKET_TAG_H
#define INT_PACKET_TAG_H

#include "ns3/tag.h"

namespace ns3 {


class Node;
class Packet;

/**
 *
 * IntPacketTag stores data related to INT 
 */
class IntPacketTag : public Tag
{
public:
  IntPacketTag ();
  virtual ~IntPacketTag ();

  /**
   * \brief Set the tag's queue data
   *
   * \param value the queue value to record
   */
  void SetQueue (uint32_t value);

  /**
   * \brief Get the tag's queue data
   *
   * \returns the queue data
   */
  uint32_t GetQueue (void) const;

  /**
   * \brief Set the tag's link data
   *
   * \param value the link data
   */
  void SetLink (uint32_t value);

  /**
   * \brief Get the tag's link data
   *
   * \returns the link data
   */
  uint32_t GetLink (void) const;

  /**
   * \brief Get the tag's link data converted to the original value
   *
   * \returns the link data 
   */
  double GetLinkConverted (void) const;

  /**
   * \brief Get the link value conversion factor
   *
   * \returns the conversion factor
   */
  uint32_t GetConversionFactor (void) const;


  /**
   * \brief Lock the tag's values
   *
   */
  void SetLock (bool value);

  /**
   * \brief Get whether the tag is locked
   *
   */
  bool GetLock (void) const;


  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:

  uint32_t m_queue {0};     //!< queue data
  uint32_t m_link {0};     //!< link data
  uint32_t m_conversionFactor {10000}; //!< conversion factor for link utilization
  bool m_lock {false};

};
} // namespace ns3

#endif /* INT_PACKET_TAG_H */
