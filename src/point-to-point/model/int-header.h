#ifndef INT_HEADER_H
#define INT_HEADER_H

#include "ns3/header.h"

namespace ns3 {

class IntHeader : public Header
{
public:

  /**
   * \brief Construct a INT header.
   */
  IntHeader ();

  /**
   * \brief Destroy a INT header.
   */
  virtual ~IntHeader ();

  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the TypeId of the instance
   *
   * \return The TypeId for this instance
   */
  virtual TypeId GetInstanceTypeId (void) const;


  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

  /**
   * \brief Set the queue data for the header
   *
   * \param value the queue data
   */
  void SetQueue (uint32_t value);

  /**
   * \brief Set the link data for the header
   *
   * \param value the link data
   */
  void SetLink (uint32_t value);

  /**
   * \brief Get the queue data for the header
   *
   * \returns the queue data
   */
  uint32_t GetQueue (void);

  /**
   * \brief Get the link data for the header
   * 
   * \returns the link data
   */
  uint32_t GetLink (void);

  /**
   * \brief Set the lock (so header cannot be changed on the rest of the path)
   *
   */
  void SetLock (bool value);

  /**
   * \brief Return whether the header is locked
   *
   */
  bool GetLock (void);

private:

  uint32_t m_queue;
  uint32_t m_link;
  bool m_lock;
};

} // namespace ns3


#endif /* INT_HEADER_H */
