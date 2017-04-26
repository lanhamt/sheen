#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <climits>
#include <cstdint>
#include <vector>
#include <algorithm>

using namespace std;

#define TIMEOUT 90

/* Default constructor */
Controller::Controller( const bool debug)
  : debug_( debug ), rtt(0), old_rtt(0), wsz(10), slow_start_thresh(500), 
    state(SLOW_START)
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{

  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) wsz;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
     << " window size is " << the_window_size << " || with rtt: " << rtt << endl;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
                                            /* of the sent datagram */
                                            const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
     << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
                               /* what sequence number was acknowledged */
                               const uint64_t send_timestamp_acked,
                               /* when the acknowledged datagram was sent (sender's clock) */
                               const uint64_t recv_timestamp_acked,
                               /* when the acknowledged datagram was received (receiver's clock)*/
                               const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
     << " received ack for datagram " << sequence_number_acked
     << " (send @ time " << send_timestamp_acked
     << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
     << endl;
  }

  rtt = (timestamp_ack_received - send_timestamp_acked);

  if (state == SLOW_START || state == FAST_RECOVERY)
    {
      if (wsz > slow_start_thresh)
        {
          slow_start_thresh = wsz / 2;
          state = CONGEST_AVOID;
          wsz += (1 / wsz);
        }
      else if (state == FAST_RECOVERY)
        {
          wsz += 2;
        }
      else
        wsz += 1;

    }
  else if (state == CONGEST_AVOID)
    {
      if (rtt >= TIMEOUT)
        {
          slow_start_thresh = wsz / 2;
          wsz = slow_start_thresh;
          state = FAST_RECOVERY;
        }
      else
        wsz += 1 / wsz;
    }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT;
}
