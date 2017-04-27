#include <iostream>

#include "controller.hh"
#include "timestamp.hh"
#include <climits>
#include <cstdint>
#include <vector>
#include <algorithm>

using namespace std;

/* Best experimentally found retransmit timer value. */
#define TIMEOUT 90

/* Best experimentally found timeout retry counter, so
   setting ssthresh to half of window size and window size
   to that value +3 happens once every TIMEOUT_RETRY tries.
   This is to avoid overreacting to consecutive delay since 
   the primary mechanism for TCP relies on a timer going off 
   when a packet is lost, we don't want to overcompensate. */
#define TIMEOUT_RETRY 8

/* Default constructor */
Controller::Controller( const bool debug)
  : debug_( debug ), 
    rtt(0),                 /* Initialize RTT. */
    wsz(13.0),              /* Initial window size, found experimentally. */
    slow_start_thresh(500), /* Initial ssthresh, found experimentally. */
    timeouts(0),            /* Timeout counter. */
    state(SLOW_START)       /* Begin in slow start state. */
{
  debug_ = false;
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if (wsz < 2)
    wsz = 2;

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
      if (rtt >= TIMEOUT)
        {
          /* When timeout occurs, do timeout adjustments if several timeouts
             have occurred and reset the counter, otherwise just increment
             the counter to avoid overreacting to delay. */
          if (timeouts < TIMEOUT_RETRY)
            timeouts++;
          else
            {
              timeouts = 0;
              slow_start_thresh = wsz / 2;
              wsz = slow_start_thresh + 3;   /* Set window to sthresh + 3 
                                                per fast recovery. */
            }
        }
      else if (wsz > slow_start_thresh)
        {
          /* Move to congestion avoidance state if window size has grown
             beyond slow start. */
          state = CONGEST_AVOID;
          wsz += (1 / wsz);
        }
      else if (state == FAST_RECOVERY)
        {
          /* Try to recover faster by doubling growth rate. */
          wsz += 2;
        }
      else
        /* Increase window size by 1 segment per RTT for initial
           slow start. */
        wsz += 1;
    }
  else if (state == CONGEST_AVOID)
    {
      if (rtt >= TIMEOUT)
        {
          /* When timeout occurs, move to fast recovery. */
          if (timeouts < TIMEOUT_RETRY)
            timeouts++;
          else
            {
              timeouts = 0;
              slow_start_thresh = wsz / 2;
              wsz = slow_start_thresh + 3;   /* Set window to sthresh + 3 
                                                per fast recovery. */
            }
          state = FAST_RECOVERY;
        }
      else
        /* For each RTT, increase by 1/wsz so window size grows 
           by 1 for each fully received window. */
        wsz += 1 / wsz;
    }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return TIMEOUT;
}
